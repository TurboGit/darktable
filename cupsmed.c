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
    http_t *http = cupsConnectDest(dest, CUPS_DEST_FLAGS_NONE/*NONE|DEVICE*/, 30000, NULL, NULL, 0, NULL, NULL);

    if (http)
    {
      const char *option_name = "media-type";
      cups_dinfo_t *dinfo = cupsCopyDestInfo (http, dest);
      ipp_attribute_t *attr = cupsFindDestSupported(http, dest, dinfo, option_name);

      if (attr)
      {
        const int count = ippGetCount(attr);
        printf("count: %d\n", count);
        const char *att_name = ippGetName(attr);
        printf("localized option: %s %s\n\n", att_name, cupsLocalizeDestOption(http, dest, dinfo, att_name));

        for (int i = 0; i < count; i ++)
        {
          const char *option_value = ippGetString(attr, i, NULL);
          printf("   format: %20s \t %s\n", option_value,
                 cupsLocalizeDestValue(http, dest, dinfo, att_name, option_value));
        }
      }
      else
        printf("no media-type found\n");
    }
    else
      printf("cannot connect to dest printer\n");
  }
}
