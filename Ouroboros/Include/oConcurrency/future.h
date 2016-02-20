// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// An approximation of C++11's std::future but with dangling calls to a more 
// complete thread scheduler than is available in the standard lib. The intent
// is for this to play well with a pre-existing work-stealing scheduler.
// GCC, VS2010, just::thread: all implement std::async as a right-now-alloc-a-
// thread-and-exec-task style system which has a lot more overhead than a more 
// robust scheduler.

#pragma once
#include <oCore/assert.h>
#include <oConcurrency/concurrency.h>
#include <oMemory/std_allocator.h>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>

// Vardiatic templates replaces this, but until they're available use callable
// macros even though it pokes back into oBase.
#include <oBase/callable.h>

namespace ouro {

enum class launch { async, deferred };
enum class future_status { ready, timeout, deferred };
enum class future_errc { broken_promise, future_already_retrieved, promise_already_satisfied, no_state, no_implementation };

const std::error_category& future_category();

/*constexpr*/ inline std::error_code make_error_code(future_errc errc) { return std::error_code(static_cast<int>(errc), future_category()); }
/*constexpr*/ inline std::error_condition make_error_condition(future_errc errc) { return std::error_condition(static_cast<int>(errc), future_category()); }

class future_error : public std::logic_error
{
public:
	future_error(future_errc errc) : logic_error(future_category().message(static_cast<int>(errc))) {}
};

template<typename T> class future;
template<typename T> class promise;

namespace future_detail {

	template<typename T> struct commitment_allocator
	{
		oDEFINE_STD_ALLOCATOR_BOILERPLATE(commitment_allocator)
		commitment_allocator() {}
		template<typename U> commitment_allocator(commitment_allocator<U> const& other) {}
		inline pointer allocate(size_type count, const_pointer hint = 0) { return static_cast<pointer>(commitment_allocate(sizeof(T) * count)); }
		inline void deallocate(pointer p, size_type count) { commitment_deallocate(p); }
	};

	oDEFINE_STD_ALLOCATOR_VOID_INSTANTIATION(commitment_allocator)
	oDEFINE_STD_ALLOCATOR_OPERATOR_EQUAL(commitment_allocator) { return true; }

	// There will be 3 permutations of commitment classes, so keep the boilerplate
	// in one place.
	#define oCOMMITMENT_NO_COPY_MOVE_CTORS(class_) \
		class_(const class_&)/* = delete*/; \
		const class_& operator=(const class_&)/* = delete*/; \
		class_(class_&&)/* = delete*/; \
		class_& operator=(class_&&)/* = delete*/;

	template<typename DerivedT>
	class commitment_base : public std::enable_shared_from_this<DerivedT>
	{
		oCOMMITMENT_NO_COPY_MOVE_CTORS(commitment_base)

	public:
		commitment_base() : task(nullptr) {}
		virtual ~commitment_base()
		{
			if (task)
			{
				delete_task_group(task);
				task = nullptr;
			}

			oAssert(is_ready(), "broken promise");
		}

		bool is_ready() const { return state.ready; }
		bool has_value() const { return state.value; }
		bool has_exception() const { return state.except; }
		bool has_future() const { return state.future; }
		bool is_deferred() const { return state.deferred; }

		void wait()
		{
			if (!is_ready())
			{
				if (task)
					task->wait();
				else
				{
					std::unique_lock<std::mutex> lock(mtx);
					while (!is_ready()) // Guarded Suspension
						CV.wait(lock);
				}
			}
		}

		template<typename Rep, typename Period>
		future_status wait_for(std::chrono::duration<Rep,Period> const& relative_time)
		{
			std::unique_lock<std::mutex> lock(mtx);
			if (task)
				throw future_error(future_errc::no_implementation);
			if (is_deferred())
				return future_status::deferred;
			while (!is_ready()) // Guarded Suspension
				if (future_status::timeout == CV.wait_for(lock, relative_time))
					return future_status::timeout;
			return future_status::ready;
		}

		void set_exception(std::exception_ptr e)
		{
			std::unique_lock<std::mutex> lock(mtx);
			if (is_ready())
				throw future_error(future_errc::promise_already_satisfied);
			ex = e;
			state.except = true;
			state.except_at_thread_exit = true;
			state.ready = true;
		}

		void set_exception_at_thread_exit(std::exception_ptr e)
		{
			std::unique_lock<std::mutex> lock(mtx);
			if (is_ready() || state.except_at_thread_exit)
				throw future_error(future_errc::promise_already_satisfied);
			ex = e;
			state.except_at_thread_exit = true;
		}

		void commit_to_task(const std::function<void()>& t)
		{
			task = new_task_group();
			task->run(t);
		}

		void set_future_attached()
		{
			std::unique_lock<std::mutex> lock(mtx);
			state.future = true;
		}

		void notify_value_set()
		{
			std::unique_lock<std::mutex> lock(mtx);
			if (is_ready())
				throw future_error(future_errc::promise_already_satisfied);
			state.value = true;
			state.value_at_thread_exit = true;
			lock.unlock();
			if (!task)
				CV.notify_all();
		}

		void notify_value_set_from_thread_exit(std::shared_ptr<commitment_base> refkeeper)
		{
			if (!is_ready()) // if we haven't fulfilled a promise, do it now
				notify_value_set();
		}

		void set_void_value()
		{
			//std::unique_lock<std::mutex> lock(mtx); // no lock required for this trivial case
			state.ready = true;
			//lock.unlock();
			if (!task)
				CV.notify_all();
		}

		template<typename T, typename U> void internal_set_value_ref(void* out_mem, const U& val)
		{
			std::unique_lock<std::mutex> lock(mtx);
			if (is_ready())
				throw future_error(future_errc::promise_already_satisfied);
			if (state.value_at_thread_exit)
				throw future_error(future_errc::no_implementation);
			*(T**)out_mem = &const_cast<U&>(val);
			state.value = true;
			state.ready = true;
			lock.unlock();
			if (!task)
				CV.notify_all();
		}

		template<typename T, typename U> void internal_set_value(void* out_mem, const U& val)
		{
			std::unique_lock<std::mutex> lock(mtx);
			if (is_ready())
				throw future_error(future_errc::promise_already_satisfied);
			if (state.value_at_thread_exit)
				throw future_error(future_errc::no_implementation);
			::new(out_mem) T(val);
			state.value = true;
			state.ready = true;
		}

		template<typename T, typename U> void internal_set_value(void* out_mem, U&& val)
		{
			std::unique_lock<std::mutex> lock(mtx);
			if (is_ready())
				throw future_error(future_errc::promise_already_satisfied);
			if (state.value_at_thread_exit)
				throw future_error(future_errc::no_implementation);
			::new(out_mem) T(std::forward<U>(val));
			state.value = true;
			state.ready = true;
		}
		
		template <typename T, typename U> void set_value_at_thread_exit(void* out_mem, U&& val)
		{
			std::unique_lock<std::mutex> lock(mtx);
			if (is_ready() || state.value_at_thread_exit)
				throw future_error(future_errc::promise_already_satisfied);
			::new(out_mem) T(std::forward<U>(val));
			if (!has_value()) // don't override a fulfilled promise
			{
				state.value_at_thread_exit = true;
				ouro::at_thread_exit(std::bind(
					commitment_base::notify_value_set_from_thread_exit, this, shared_from_this()));
			}
		}

		template <typename T, typename U> void set_value_at_thread_exit_ref(void* out_mem, const U& val)
		{
			std::unique_lock<std::mutex> lock(mtx);
			if (is_ready() || state.value_at_thread_exit)
				throw future_error(future_errc::promise_already_satisfied);
			*(T**)out_mem = &const_cast<U&>(val);
			if (!has_value()) // don't override a fulfilled promise
			{
				state.value_at_thread_exit = true;
				ouro::at_thread_exit(std::bind(
					commitment_base::notify_value_set_from_thread_exit, this, shared_from_this()));
			}
		}

protected:
		struct state_t
		{
			state_t() { static_assert(sizeof(state_t) == sizeof(char), "size mismatch"); *(char*)this = 0;  }
			bool value:1;
			bool value_at_thread_exit:1;
			bool except:1;
			bool except_at_thread_exit:1;
			bool future:1;
			bool deferred:1;
			bool ready:1;
		};

		task_group* task;
		mutable std::mutex mtx;
		std::condition_variable CV;
		state_t state;
		std::exception_ptr ex;
	};

	template<typename T> class commitment_t : public commitment_base<commitment_t<T>>
	{
		oCOMMITMENT_NO_COPY_MOVE_CTORS(commitment_t)
	public:
		commitment_t() {}
		~commitment_t() {}
		template <typename U> void set_value(const U& val) { internal_set_value<T>(&value, val); }
		template <typename U> void set_value(U&& val) { internal_set_value<T>(&value, std::move(val)); }
		template <typename U> void set_value_at_thread_exit(const U& val) { set_value_at_thread_exit<T>(&value, val); }
		template <typename U> void set_value_at_thread_exit(U&& val) { set_value_at_thread_exit<T>(&value, std::move(val)); }

		T move_value()
		{
			wait();
			if (has_exception())
				std::rethrow_exception(ex);
			return std::move(*reinterpret_cast<T*>(&value));
		}

	private:
		T value;
	};

	template<typename T> class commitment_t<T&> : public commitment_base<commitment_t<T&>>
	{
		oCOMMITMENT_NO_COPY_MOVE_CTORS(commitment_t)
	public:
		commitment_t() {}
		~commitment_t() {}

		template <typename U> void set_value(const U& value) { internal_set_value_intrusive_ptr<T>(&value, value); }
		template <typename U> void set_value_at_thread_exit(const U& value) { set_value_at_thread_exit_intrusive_ptr<T>(&value, value); }

		typename std::add_lvalue_reference<T>::type copy_value()
		{
			wait();
			if (has_exception())
				std::rethrow_exception(ex);
			return *value;
		}

	private:
		typedef T* storage_type;
		storage_type value;
	};

	template<> class commitment_t<void> : public commitment_base<commitment_t<void>>
	{
		oCOMMITMENT_NO_COPY_MOVE_CTORS(commitment_t)
	public:
		commitment_t() {}
		~commitment_t() {}
	
		void void_value()
		{
			wait();
			if (has_exception())
				std::rethrow_exception(ex);
		}
	};

	#define oFUTURE_VALIDATE() do { if (!valid()) throw future_error(future_errc::no_state); } while(false)

	template<typename DerivedT> class future_base
	{
	public:
		inline bool valid() const { return !!This()->commitment; }
		inline bool is_ready() const { oFUTURE_VALIDATE(); return This()->commitment->is_ready(); }
		inline bool has_value() const { oFUTURE_VALIDATE(); return This()->commitment->has_value(); }
		inline bool has_exception() const { oFUTURE_VALIDATE(); return This()->commitment->has_exception(); }
		inline bool is_deferred() const { oFUTURE_VALIDATE(); return This()->commitment->is_deferred(); }
		inline void wait() { oFUTURE_VALIDATE(); This()->commitment->wait(); }

		template<typename Rep, typename Period>
		future_status wait_for(std::chrono::duration<Rep,Period> const& relative_time)
		{
			oFUTURE_VALIDATE(); 
			return This()->commitment->wait_for(relative_time);
		}

		template<typename Clock, typename Duration>
		future_status wait_until(std::chrono::time_point<Clock,Duration> const& absolute_time)
		{
			oFUTURE_VALIDATE(); 
			chrono::high_resolution_clock::duration duration = 
				time_point_cast<chrono::high_resolution_clock::time_point>(absolute_time) 
				- chrono::high_resolution_clock::now();
			return wait_for(duration);
		}
		
	private:
		DerivedT* This() { return static_cast<DerivedT*>(this); }
		const DerivedT* This() const { return static_cast<const DerivedT*>(this); }
	};

	#define oFUTURE_CHECK(expr, errc) do { if (!(expr)) throw future_error(future_errc::errc); } while(false)

	// @tony revisit this:
	// The spec says there are no copy-constructors... but I really need to pass 
	// this to a std::bind. 
	// There is a bug in MSVC2010 that disallows std::move through std::bind:
	// http://connect.microsoft.com/VisualStudio/feedback/details/649274/std-bind-and-std-function-are-not-move-aware 
	// So until then, use a loophole in the spec: There are no copy-ctors for 
	// future or promise objects. Thus USE a copy ctor, const_cast it away and 
	// eviscerate the source.
	#define oFUTURE_MOVE_CTOR(class_) \
		class_(const class_& that) : commitment(std::move(const_cast<class_&>(that).commitment)) {} \
		class_& operator=(const class_& that) { if (this != &that) { commitment = std::move(const_cast<class_&>(that).commitment); } return *this; } \
		class_(class_&& that) : commitment(std::move(that.commitment)) {} \
		class_& operator=(class_&& that) { if (this != &that) { commitment = std::move(that.commitment); } return *this; }

	// Certain base classes, permutations of the same concept, and special-case
	// interfaces need direct access to the commitment, or act as a factor for a 
	// future, so centralize this access here between the template types for future.
	#define oFUTURE_COMMON(commitment_type) public: future() {} ~future() {} oFUTURE_MOVE_CTOR(future) \
		future(const std::shared_ptr<future_detail::commitment_t<commitment_type>>& c) : commitment(c) \
		{	if (commitment->has_future()) \
				throw future_error(future_errc::future_already_retrieved); \
			commitment->set_future_attached(); \
		} \
		protected: std::shared_ptr<future_detail::commitment_t<commitment_type>> commitment; \
		template <typename> friend class future_detail::future_base; \
		template <typename> friend class promise; \
		template <typename> friend class shared_future; \
		template <typename> friend class packaged_task; \

	#define oPROMISE_CTORS() ~promise() { if (commitment && !commitment->is_ready() && commitment->has_future()) oAssert(0, "broken promise"); } oFUTURE_MOVE_CTOR(promise)

	#define oPROMISE_SET_EXCEPTION() \
		void set_exception(std::exception_ptr e) { if (!commitment) throw future_error(future_errc::no_state); commitment->set_exception(e); } \
		void set_exception_at_thread_exit(std::exception_ptr e) { if (!commitment) throw future_error(future_errc::no_state); commitment->set_exception_at_thread_exit(e); } \

	#define oPROMISE_COMMON(Type) \
		private: std::shared_ptr<future_detail::commitment_t<Type>> commitment; \
		public: promise(const std::shared_ptr<future_detail::commitment_t<Type>>& c) : commitment(c) {} \
		public: promise() { commitment = std::allocate_shared<future_detail::commitment_t<Type>>(future_detail::commitment_allocator<future_detail::commitment_t<Type>>()); } \
		public: oPROMISE_CTORS(); \
		public: oPROMISE_SET_EXCEPTION(); \
		public: future<Type> get_future() { if (commitment->has_future()) throw future_error(future_errc::future_already_retrieved); return future<Type>(commitment); }

	// The spec says there are no copy-constructors for packaged_task but MSVC2010 casting packaged_task to a std::function 
	// is not move aware (and uses the copy constructor):
	// http://connect.microsoft.com/VisualStudio/feedback/details/649274/std-bind-and-std-function-are-not-move-aware 
	// So for now enable the copy constructor that's needed, but let it behave like a move constructor.
	#define oPACKAGED_TASK_MOVE_CTORS() \
		public: packaged_task(packaged_task&& that) : commitment(std::move(that.commitment)), func(std::move(that.func)) {} \
		public: packaged_task(const packaged_task& that) : commitment(std::move(const_cast<packaged_task&>(that).commitment)), func(std::move(const_cast<packaged_task&>(that).func)) {} \
		public: packaged_task& operator=(packaged_task&& that) { if (this != &that) { commitment = std::move(that.commitment); func = std::move(that.func); } return *this; }

	#define oPACKAGED_TASK_CTORS() \
		oPACKAGED_TASK_MOVE_CTORS() \
		private: packaged_task& operator=(packaged_task const&)/* = delete*/; \
		public: packaged_task() {} ~packaged_task() {}

} // namespace detail

template<typename T> class future : public future_detail::future_base<future<T>>
{
	oFUTURE_COMMON(T)
public:
	T get() { oFUTURE_VALIDATE(); return commitment->move_value(); }
};

template <class T> class future<T&> : public future_detail::future_base<future<T&>>
{
	oFUTURE_COMMON(T&)
public:
	T& get() { oFUTURE_VALIDATE(); return commitment->copy_value(); }
};

template<> class future<void> : public future_detail::future_base<future<void>>
{
	oFUTURE_COMMON(void)
public:
	void get() { commitment->void_value(); }
};

template<typename T> class promise
{
	oPROMISE_COMMON(T);
public:
	void set_value(const T& value) { oFUTURE_CHECK(commitment, no_state); commitment->set_value(value); }
	void set_value(T&& value) { oFUTURE_CHECK(commitment, no_state); commitment->set_value(std::move(value)); }
	void set_value_at_thread_exit(const T& value) { oFUTURE_CHECK(commitment, no_state); commitment->set_value_at_thread_exit(value); }
	void set_value_at_thread_exit(T&& value) { oFUTURE_CHECK(commitment, no_state); commitment->set_value_at_thread_exit(std::move(value)); }
};

template <class T> class promise<T&>
{
	oPROMISE_COMMON(T&);
public:
	void set_value(const T& value) { oFUTURE_CHECK(commitment, no_state); commitment->set_value(value); }
	void set_value_at_thread_exit(const T& value) { oFUTURE_CHECK(commitment, no_state); commitment->set_value_at_thread_exit(value); }
};

template<> class promise<void>
{
	oPROMISE_COMMON(void);
public:
	void set_value() { commitment->set_void_value(); }
};

namespace future_detail {
	template<typename result_type> 
	inline void fulfill_promise_helper(promise<result_type>& p, std::function<result_type(void)>& f) { p.set_value(f()); }
	template<> 
	inline void fulfill_promise_helper<void>(promise<void>& p, std::function<void(void)>& f) { f(); p.set_value(); }
};

#ifdef oHAS_VARIADIC_TEMPLATES
	#error reimplement packaged_task using variadic templates
#else
	oCALLABLE_TEMPLATE0
	class packaged_task {};

	#define oPACKAGEDTASK(nargs) \
	template<typename result_type oCALLABLE_CONCAT(oARG_COMMA_TYPENAMES,nargs)> \
	class packaged_task<result_type(oCALLABLE_CONCAT(oARG_PARTIAL_TYPENAMES,nargs))> \
	{ \
	public: \
		oPACKAGED_TASK_CTORS() \
		typedef std::function<result_type(oCALLABLE_CONCAT(oARG_PARTIAL_TYPENAMES,nargs))> Callable; \
		\
		explicit packaged_task(oCALLABLE_PARAMS0) \
		{ \
			commitment = std::allocate_shared<future_detail::commitment_t<result_type>>(future_detail::commitment_allocator<future_detail::commitment_t<result_type>>()); \
			func = std::move(oCALLABLE_PASS0); \
		} \
		\
		bool valid() const { return !!func; } \
		void swap(packaged_task& _Other) { if (this != &_Other) { std::swap(commitment, _Other.commitment); std::swap(func, _Other.func); } } \
		\
		future<result_type> get_future() { oFUTURE_CHECK(!commitment->has_future(), future_already_retrieved); return std::move(future<result_type>(commitment)); } \
		\
		void operator ()(oCALLABLE_CONCAT(oARG_DECL,nargs)) \
		{ \
			std::function<result_type()> function1 = std::tr1::bind(func oCALLABLE_CONCAT(oARG_COMMA_PASS,nargs)); \
			fulfill_promise(std::move(promise<result_type>(commitment)), function1); \
		} \
		\
		void commit_to_task(oCALLABLE_CONCAT(oARG_DECL,nargs)) \
		{ \
			std::function<result_type()> function1 = std::tr1::bind(func oCALLABLE_CONCAT(oARG_COMMA_PASS,nargs)); \
			std::function<void()> function2 = std::tr1::bind(packaged_task::fulfill_promise, std::move(promise<result_type>(commitment)), function1); \
			commitment.get()->commit_to_task(function2); \
		} \
		\
		void reset() { packaged_task(std::move(func)).swap(*this); } \
		\
	private: \
		std::shared_ptr<future_detail::commitment_t<result_type>> commitment; \
		Callable func; \
		\
		static void fulfill_promise(promise<result_type> p, std::function<result_type(void)> f) \
		{ try { future_detail::fulfill_promise_helper(p, f); } catch (std::exception&) { p.set_exception(std::current_exception()); } } \
	};
	oCALLABLE_PROPAGATE(oPACKAGEDTASK)
#endif

// future<result_of(callable)> async(callable, args...);
#ifdef oHAS_VARIADIC_TEMPLATES
	#error reimplement async using variadic templates
#else
	#define oASYNC(nargs) oCALLABLE_CONCAT(oCALLABLE_TEMPLATE,nargs) future<oCALLABLE_CONCAT(oCALLABLE_RETURN_TYPE,nargs)> async(oCALLABLE_CONCAT(oCALLABLE_PARAMS,nargs)) \
	{	packaged_task<oCALLABLE_CONCAT(oCALLABLE_RETURN_TYPE,nargs)(oCALLABLE_CONCAT(oARG_PARTIAL_TYPENAMES,nargs))> p(oCALLABLE_PASS0); \
		p.commit_to_task(oCALLABLE_CONCAT(oARG_PASS,nargs)); \
		return std::move(p.get_future()); \
	}
	oCALLABLE_PROPAGATE(oASYNC)
#endif

}
