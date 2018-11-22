/*
  $ gcc -g -o cupsmed cupsmed.c -lcups && ./cupsmed PDF
*/

#include <stdio.h>
#include <cups/cups.h>

void main(int argc, char *argv[])
{
  if(argc==1)
  {
    printf("usage: %s <printer-name>\n", argv[0]);
    return;
  }

  const char *printer_name = argv[1];

  cups_dest_t *dests;
  int num_dests = cupsGetDests(&dests);
  cups_dest_t *dest = cupsGetDest(printer_name, NULL, num_dests, dests);

  if (!dest)
  {
    printf("cannot get dest printer\n");
    return;
  }

  const char *uri = cupsGetOption("printer-uri-supported", dest->num_options, dest->options);

  printf("**** PRINTER [%s]    URI:[%s]\n", printer_name, uri);

  dest = cupsGetDestWithURI(NULL, uri);

  if (dest)
  {
    char resource[256];
    http_t *http = cupsConnectDest(dest, CUPS_DEST_FLAGS_NONE/*NONE|DEVICE*/, 30000, NULL, resource, sizeof(resource), NULL, NULL);

    ipp_t *request = ippNewRequest(IPP_OP_GET_PRINTER_ATTRIBUTES);
    ipp_t *response = cupsDoRequest(http, request, resource);

    ipp_attribute_t *attr;

    if ((attr = ippFindAttribute(response, "printer-state",
                                 IPP_TAG_ENUM)) != NULL)
    {
      printf("printer-state=%s\n",
             ippEnumString("printer-state", ippGetInteger(attr, 0)));
    }
    else
      puts("printer-state=unknown");

    if ((attr = ippFindAttribute(response, "printer-state-message",
                                 IPP_TAG_TEXT)) != NULL)
    {
      printf("printer-state-message=\"%s\"\n",
             ippGetString(attr, 0, NULL));
    }

    if ((attr = ippFindAttribute(response, "media-type", IPP_TAG_NAME)) != NULL)
    {
      printf("media-type=\"%s\"\n",  ippGetString(attr, 0, NULL));
    }

    if ((attr = ippFindAttribute(response, "printer-state-reasons",
                                 IPP_TAG_KEYWORD)) != NULL)
    {
      int i, count = ippGetCount(attr);

      puts("printer-state-reasons=");
      for (i = 0; i < count; i ++)
        printf("    %s\n", ippGetString(attr, i, NULL));
    }

  }
}
