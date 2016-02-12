Option Explicit

'
' Swaps the values at the specified indices in the specified array
' (This is required by QSort)
'
Sub Swap(Array, Index1, Index2)
	Dim Temp
	Temp = Array(Index1)
	Array(Index1) = Array(Index2)
	Array(Index2) = Temp
End Sub

'
' Helper function for QSort
'
Function QSortInternalPartitionInt(IndexArray, Array, iFirstIndex, iLastIndex, iPivotIndex)
	Dim iStoreIndex, PivotValue, i
	PivotValue = Array(IndexArray(iPivotIndex))
	Swap IndexArray, iPivotIndex, iLastIndex
	iStoreIndex = iFirstIndex
	For i = iFirstIndex To iLastIndex - 1
		'
		' NOTE: Here's the comparison. Right now this converts the specified value to an 
		' int for comparison, which may not be appropriate for another particular usage.
		' If only vbscript could pass function pointers!
		'
		If CInt(Array(IndexArray(i))) < CInt(PivotValue) then
			Swap IndexArray, i, iStoreIndex
			iStoreIndex = iStoreIndex + 1
		End If
	Next
	Swap IndexArray, iStoreIndex, iLastIndex
	QSortInternalPartitionInt = iStoreIndex
End Function

'
' The internal version of QSort that is called recursively
'
Sub QSortInternalInt(IndexArray, Array, iFirstIndex, iLastIndex)
	If iFirstIndex < iLastIndex Then 
		Dim iPivotIndex, iPivotNewIndex
		iPivotIndex = Round(iFirstIndex + (iLastIndex - iFirstIndex) / 2, 0)
		'msgbox "iPivotIndex="&iPivotIndex
		iPivotNewIndex = QSortInternalPartitionInt(IndexArray, Array, iFirstIndex, iLastIndex, iPivotIndex)
		
		'msgbox "qs(" & iFirstIndex & ", " & iPivotNewIndex - 1 & ")"
		QSortInternalInt IndexArray, Array, iFirstIndex, iPivotNewIndex - 1
		'msgbox "qs(" & iPivotNewIndex + 1 & ", " & iLastIndex & ")"
		QSortInternalInt IndexArray, Array, iPivotNewIndex + 1, iLastIndex
	End If
End Sub

'
' Returns a new array that contains indices into the specified array to access them
' for a sorted order. This was done so that pairs or multi-arrays can be accessed
' since otherwise their link-by-index is broken if one channel is sorted.
'
Function QSort(Array)
	Dim i
	Redim IndexArray(UBound(Array))
	For i = LBound(IndexArray) To UBound(IndexArray)
		IndexArray(i) = i
	Next

	QSortInternalInt IndexArray, Array, LBound(Array), UBound(Array)
	QSort = IndexArray
End Function

'
' Return a sorted dictionary by sorting its values as if they were int values
' This form is provided because keys must be unique, and maybe the int values
' aren't.
'
Function QSortByIntItems(DictWithIntKey)
	Dim OrderedDict
	Set OrderedDict = CreateObject("Scripting.Dictionary")
	Dim IntItems
	IntItems = DictWithIntKey.Items()
	QSortInt IntItems
	
	For i = LBound(IntItems) To UBound(IntItems)
		OrderedDict.Add IntKeys(i), DictWithIntKey.Item(IntKeys(i))
	Next
	Set QSortByIntKey = OrderedDict
End Function

'
' Collections are very annoying in VBScript, so convert it
' here to an array.
'
Function ScriptGetArgumentArray()

	Dim dict
	Set dict = CreateObject("Scripting.Dictionary")

	Dim args
	Set args = WScript.Arguments

	Dim arg
	Dim i
	i = 0

	For Each arg In args
		dict.Add i, arg
		i = i + 1
	Next

	Dim items
	items = dict.Items()

	ScriptGetArgumentArray = items

End Function

Function ArrayFromString(string, strToken)

	Dim dict
	Set dict = CreateObject("Scripting.Dictionary")

	Dim lstring
	lstring = string

	Dim pos
	pos = InStr(1, lstring, strToken)

	Dim index
	index = 0

	Do While pos <> 0
		Dim token
		token = Left(lstring, pos - 1)
		lstring = Right(lstring, Len(lstring) - (pos + Len(strToken) - 1))
		dict.Add index, token
		index = index + 1
		pos = InStr(1, lstring, strToken)
	Loop

	If Len(lstring) > 0 Then
		dict.Add index, lstring
	End If

	Dim arr
	arr = dict.Items
	ArrayFromString = arr
End Function

'
' Executes the specified command line for P4 and returns the resulting stdout 
' as a string.
'
Function P4Execute(StrP4Command)
	Dim Shell, Exec, StrStdOut
	Set Shell = WScript.CreateObject("WScript.Shell")
	Set Exec = Shell.Exec("p4 " & StrP4Command)
	P4Execute = Exec.StdOut.ReadAll()
End Function

'
' Returns an array of strings of P4 users names
'
Function P4GetUsers()
	Dim StrStdOut, ArrUsers, i, p
	StrStdOut = P4Execute("users")
	ArrUsers = ArrayFromString(StrStdOut, vbNewline)
	For i = LBound(ArrUsers) To UBound(ArrUsers)
		p = InStr(1, ArrUsers(i), " <")
		ArrUsers(i) = Left(ArrUsers(i), p)
	Next
	P4GetUsers = ArrUsers
End Function

'
' Returns an array of strings for each line/changelist as returned from 
' a p4 changelists -s submitted -u <StrUsername> command
'
Function P4GetSubmittedChangelists(StrUsername)
	Dim StrStdOut
	StrStdOut = P4Execute("changelists -s submitted -u " & StrUsername)
	P4GetSubmittedChangelists = ArrayFromString(StrStdOut, vbNewline)
End Function

'
' Returns an array of strings for each line/changelist as returned from 
' a p4 changelists -s submitted -u <StrUsername> command, but between the 
' specified dates. (Remember the Date params are of date type, not string)
'
Function P4GetSubmittedChangelistsDated(StrUsername, DateSince, DateUpTo)
	Dim Dict, DictIndex, ArrStrSubmitted, i, DateSubmitted
	Set Dict = CreateObject("Scripting.Dictionary")
	DictIndex = 0
	ArrStrSubmitted = P4GetSubmittedChangelists(StrUsername)
	For i = LBound(ArrStrSubmitted) To UBound(ArrStrSubmitted)
		DateSubmitted = P4GetSubmittedDate(ArrStrSubmitted(i))
		If DateSubmitted >= DateSince And DateSubmitted <= DateUpTo Then
			Dict.Add DictIndex, ArrStrSubmitted(i)
			DictIndex = DictIndex + 1
		End If
	Next
	P4GetSubmittedChangelistsDated = dict.Items
End Function

'
' Returns date type for the day/month/year a changelist was submitted as parsed
' from the string generated by a p4 changelists -s submitted command
'
Function P4GetSubmittedDate(StrChangelistLine)
	Dim iPosOn, iPosBy
	iPosOn = InStr(1, StrChangelistLine, "on ")
	iPosBy = InStr(iPosOn, StrChangelistLine, "by")
	P4GetSubmittedDate = DateValue(Mid(StrChangelistLine, iPosOn + 3, iPosBy - iPosOn - 4))
End Function

'
' Returns the number of changelists the specified user has ever submitted
'
Function P4GetSubmittedChangelistCount(StrUsername)
	Dim ArrStrSubmitted
	ArrStrSubmitted = P4GetSubmittedChangelists(StrUsername)
	P4GetSubmittedChangelistCount = UBound(ArrStrSubmitted) + 1
End Function

'
' Returns the number of changelists the specified user has submitted between the
' specified dates (remember data params should be date types/objects)
'
Function P4GetSubmittedChangelistCountDated(StrUsername, DateSince, DateUpTo)
	Dim ArrStrSubmitted
	ArrStrSubmitted = P4GetSubmittedChangelistsDated(StrUsername, DateSince, DateUpTo)
	P4GetSubmittedChangelistCountDated = UBound(ArrStrSubmitted) + 1
End Function

Sub Main()
	Dim Dict, ArrUsers, StrSinceDate, StrSummary, i, DictSortedBySubmitCount
	Set Dict = CreateObject("Scripting.Dictionary")
	ArrUsers = P4GetUsers()
	StrSinceDate = ""

    If Len(StrSinceDate) = 0 Then
        Dim args
        args = ScriptGetArgumentArray()
        If UBound(args) <> -1 Then
            StrSinceDate = args(0)
        End If
    End If

	If Len(StrSinceDate) = 0 Then
		For i = LBound(ArrUsers) To UBound(ArrUsers)
			Dict.Add ArrUsers(i), P4GetSubmittedChangelistCount(ArrUsers(i))
		Next
	Else
		For i = LBound(ArrUsers) To UBound(ArrUsers)
			Dict.Add ArrUsers(i), P4GetSubmittedChangelistCountDated(ArrUsers(i), DateValue(StrSinceDate), Date)
		Next
	End If

	If Len(StrSinceDate) = 0 Then StrSinceDate = "the beginning"
	StrSummary = "Here are submissions by P4 users since " & StrSinceDate & ":" & vbNewline
	
	Dim Keys, Items, Indices
	Keys = Dict.Keys()
	Items = Dict.Items()
	Indices = QSort(Items)
	For i = UBound(Indices) To LBound(Indices) Step -1
		StrSummary = StrSummary & Keys(Indices(i)) & ": " & vbTab & Items(Indices(i)) & vbNewline
	Next
	MsgBox StrSummary, vbInformation, "P4 Submissions Summary"
End Sub

Main
