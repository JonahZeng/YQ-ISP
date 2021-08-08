#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

static void exampleFunc(const char *filename) {
    xmlParserCtxtPtr ctxt;
    xmlDocPtr doc; 

    ctxt = xmlNewParserCtxt();
    if (ctxt == NULL) {
        fprintf(stderr, "Failed to allocate parser context\n");
        return;
    }
    doc = xmlCtxtReadFile(ctxt, filename, NULL, XML_PARSE_NOBLANKS);

    if (doc == NULL) {
        fprintf(stderr, "Failed to parse %s\n", filename);
        xmlFreeParserCtxt(ctxt);
        return;
    }

    if (ctxt->valid == 0)
    {
        fprintf(stderr, "Failed to validate %s\n", filename);
        xmlFreeParserCtxt(ctxt);
        return;
    }
    xmlNodePtr cur = xmlDocGetRootElement(doc);
    if (cur == NULL)
    {
        fprintf(stderr, "empty document\n");
        xmlFreeDoc(doc);
        xmlFreeParserCtxt(ctxt);
        return;
    }

    printf("cur name: %s, type:%d\n", cur->name, cur->type);
    cur = cur->children;
    while (cur != NULL)
    {
        printf("first child name %s\n", cur->name);
        cur = cur->next;
    }
    /*
    XML_ELEMENT_NODE=		1,
    XML_ATTRIBUTE_NODE=		2,
    XML_TEXT_NODE=		3,
    XML_CDATA_SECTION_NODE=	4,
    XML_ENTITY_REF_NODE=	5,
    XML_ENTITY_NODE=		6,
    XML_PI_NODE=		7,
    XML_COMMENT_NODE=		8,
    XML_DOCUMENT_NODE=		9,
    XML_DOCUMENT_TYPE_NODE=	10,
    XML_DOCUMENT_FRAG_NODE=	11,
    XML_NOTATION_NODE=		12,
    XML_HTML_DOCUMENT_NODE=	13,
    XML_DTD_NODE=		14,
    XML_ELEMENT_DECL=		15,
    XML_ATTRIBUTE_DECL=		16,
    XML_ENTITY_DECL=		17,
    XML_NAMESPACE_DECL=		18,
    XML_XINCLUDE_START=		19,
    XML_XINCLUDE_END=		20,
    XML_DOCB_DOCUMENT_NODE=	21
    */

    xmlFreeParserCtxt(ctxt);
}

int main(int argc, char* argv[])
{
    LIBXML_TEST_VERSION

    const char* xml_file_name = "../cfg/V1_config.xml";
    exampleFunc(xml_file_name);

    /*
     * Cleanup function for the XML library.
     */
    xmlCleanupParser();
    /*
     * this is to debug memory for regression tests
     */
    xmlMemoryDump();
    return 0;
}