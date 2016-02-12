// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oString/xml.h>
#include <memory>

using namespace ouro;

static const char* sExpectedArtists[] = { "Bob Dylan", "Bonnie Tyler", "Dolly Parton", };

static const char* sTestXML = {
	"<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>" \
	"<!--  Edited by XMLSpy -->" \
	"<CATALOG title=\"My play list\" count=\"3\">" \
	"	<CD>" \
	"		<TITLE>Empire Burlesque</TITLE>" \
	"		<ARTIST>Bob Dylan</ARTIST>" \
	"		<COUNTRY>USA</COUNTRY>" \
	"		<COMPANY>Columbia</COMPANY>" \
	"		<PRICE>10.90</PRICE>" \
	"		<YEAR>1985</YEAR>" \
	"	</CD>" \
	"	<SpecialCD>" \
	"		<TITLE>Hide your heart</TITLE>" \
	"		<ARTIST>Bonnie Tyler</ARTIST>" \
	"		<COUNTRY>UK</COUNTRY>" \
	"		<COMPANY>CBS Records</COMPANY>" \
	"		<PRICE>9.90</PRICE>" \
	"		<YEAR>1988</YEAR>" \
	"	</SpecialCD>" \
	"	<CD>" \
	"		<TITLE>Greatest Hits</TITLE>" \
	"		<ARTIST>Dolly Parton</ARTIST>" \
	"		<COUNTRY>USA</COUNTRY>" \
	"		<COMPANY>RCA</COMPANY>" \
	"		<PRICE>9.90</PRICE>" \
	"		<YEAR>1982</YEAR>" \
	"	</CD>" \
	"</CATALOG>" \
	"<TEST booleanattr test='123' anotherboolattr test2='123' boolattr3 />" \
	"<TEST booleanattr test='123' anotherboolattr test2='123' boolattr3>test</TEST>"
};

static const char* sCompactTestXML = "<html><head><title>foo bar</title></head><body/></html>";

static const char* sCompactExpectedVisitOrder = "html, head, title, title; foo bar, title, head, body, body, html, ";

class test_visitor : public xml::visitor
{
public:
	bool node_begin(const char* _NodeName, const char* _XRef, const attr_type* _pAttributes, size_t _NumAttributes) override { s += _NodeName; s += ", "; return true; }
 	bool node_end(const char* _NodeName, const char* _XRef) override { s += _NodeName; s += ", "; return true; }
	bool node_text(const char* _NodeName, const char* _XRef, const char* _NodeText) override { s += _NodeName; s += "; "; s += _NodeText; s += ", "; return true; }

	std::string s;
};

oTEST(oString_xml)
{
	std::shared_ptr<xml> XML = std::make_shared<xml>("Test XML", sTestXML, 200);

	xml::node hCatalog = XML->first_child(XML->root(), "CATALOG");
	oCHECK(hCatalog, "Cannot find CATALOG node");
	oCHECK(!strcmp(XML->find_attr_value(hCatalog, "title"), "My play list"), "CATALOG title is incorrect");

	int count = atoi(XML->find_attr_value(hCatalog, "count"));
	oCHECK(count == 3, "Failed to get a valid count");

	int i = 0;
	for (xml::node n = XML->first_child(hCatalog); n; n = XML->next_sibling(n), i++)
	{
		oCHECK(!strcmp(XML->node_name(n), i == 1 ? "SpecialCD" : "CD"), "Invalid node name %s", XML->node_name(n));
		xml::node hArtist = XML->first_child(n, "ARTIST");
		oCHECK(hArtist, "Invalid CD structure");
		oCHECK(!strcmp(XML->node_value(hArtist), sExpectedArtists[i]), "Artist in %d%s section did not match", i, ordinal(i));
	}
	 
	// Test xref
	// @tony: WTF: if this is xml::node n = ... then all kinds of havok 
	// happens. I traced it through the register moves on the function return, and
	// it's all ok until it gets moved... to a wrong location! Somehow VC is 
	// confused and thinks this is still the xml::node n from above in the for
	// loop! This has to be a compiler bug.
	xml::node m = XML->root();
	const char* name = XML->node_name(m);
	m = XML->first_child(m);
	name = XML->node_name(m);
	m = XML->first_child(m);
	name = XML->node_name(m);
	m = XML->next_sibling(m);
	name = XML->node_name(m);
	m = XML->first_child(m);
	name = XML->node_name(m);
	m = XML->next_sibling(m);
	name = XML->node_name(m);
	m = XML->next_sibling(m);
	name = XML->node_name(m);
	const char* val = XML->node_value(m);

	mstring xref;
	XML->make_xref(xref, m);

	xml::node xref_m = XML->find_xref(xref);
	const char* v = XML->node_value(xref_m);
	oCHECK(!_stricmp("UK", v), "failed: Xref %s", xref.c_str());

	// Test boolean attrs

	xml::node nTest1 = XML->find_xref("/2");
	oCHECK(nTest1, "Did not find first boolean attr test");

	val = XML->find_attr_value(nTest1, "anotherboolattr");
	oCHECK(!strcmp(val, "anotherboolattr"), "anotherboolattr failed");
	val = XML->find_attr_value(nTest1, "not_there");
	oCHECK(!val || !strcmp(val, ""), "not_there should really not be there, but something was: %s", val);
	val = XML->find_attr_value(nTest1, "boolattr3");
	oCHECK(!strcmp(val, "boolattr3"), "boolattr3 failed");

	xml::node nTest2 = XML->find_xref("/3");
	oCHECK(nTest2, "Did not find second boolean attr test");
	val = XML->find_attr_value(nTest2, "boolattr3");
	oCHECK(!strcmp(val, "boolattr3"), "boolattr3 2 failed");

	// Test compacted XML
 	XML = std::make_shared<xml>("Test CompactXML", sCompactTestXML, 200);

	xml::node HeadNode = XML->first_child(XML->first_child(XML->root()), "head");
	oCHECK(!_stricmp(XML->node_name(HeadNode), "head"), "Failed to get head node");
	
	xml::node Title = XML->first_child(HeadNode, "title");
	oCHECK(!_stricmp(XML->node_value(Title), "foo bar"), "Title is wrong");

	test_visitor t;
	XML->visit(t);
	oCHECK(!strcmp(t.s.c_str(), sCompactExpectedVisitOrder), "Visit out of order: %s", t.s.c_str());
}
