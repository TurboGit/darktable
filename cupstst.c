//
// $ gcc -g -o cupstst cupstst.c -lcups && ./cupstst
//

#include <stdio.h>
#include <cups/cups.h>
#include <cups/ppd.h>

#ifndef CUPS_DEST_FLAGS_DEVICE
// was introduced in 2.2.4
#  define CUPS_DEST_FLAGS_DEVICE CUPS_DEST_FLAGS_NONE
#endif /* !CUPS_DEST_FLAGS_DEVICE */

char printer_uri[1024];

void media_type_ppd(const char *printer_name)
{
  const char *PPDFile = cupsGetPPD(printer_name);
  ppd_file_t *ppd = ppdOpenFile(PPDFile);

  if (ppd)
  {
    ppd_option_t *opt = ppdFindOption(ppd, "MediaType");

    if (opt)
      {
        ppd_choice_t *choice = opt->choices;

        for (int k=0; k<opt->num_choices; k++)
          {
            printf ("PPD: media type %s | %s\n", choice->choice, choice->text);
            choice++;
          }
      }

    ppdClose(ppd);
    unlink(PPDFile);
  }
}

void media_type(cups_dest_t *dest, http_t *hcon, cups_dinfo_t *info)
{
  ipp_attribute_t *attr = cupsFindDestSupported(hcon, dest, info, CUPS_MEDIA_TYPE);

  if (attr)
    {
      printf("some attributes...%d, %s\n", ippGetCount(attr), ippGetName(attr));
      for (int k=0; k<ippGetCount(attr); k++)
        {
          const char *value = ippGetString(attr, k, NULL);
          printf ("   %s | %s | %s\n", value, cupsLocalizeDestOption(hcon, dest, info, value),
                  cupsLocalizeDestValue(hcon, dest, info, "media-type", value));
        }
    }
  else
    {
      if (cupsCheckDestSupported(hcon, dest, info, CUPS_MEDIA_TYPE, NULL))
        if ((attr = cupsFindDestSupported(hcon, dest, info, CUPS_MEDIA_TYPE)) != NULL)
          {
            const int count = ippGetCount(attr);
            printf ("found count %d\n", count);
          }
    }
}

////////////////////////////////////////////// IS TURBOPRINT
void is_turboprint_ppd(const char *printer_name)
{
  const char *PPDFile = cupsGetPPD (printer_name);
  ppd_file_t *ppd = ppdOpenFile(PPDFile);

  if (ppd)
    {
      ppd_attr_t *attr = ppdFindAttr(ppd, "ModelName", NULL);

      if (attr)
        {
          printf ("PPD: is-turboprint %d\n", strstr(attr->value, "TurboPrint") == NULL ? 0 : 1);
        }
    }

  ppdClose(ppd);
  unlink(PPDFile);
}
void is_turboprint(cups_dest_t *dest, http_t *hcon, cups_dinfo_t *info)
{
  // only works with cupsGetDest()
  const char *attr = cupsGetOption("printer-make-and-model", dest->num_options, dest->options);

  if (attr)
    {
      printf ("IPP: is-turboprint %d\n", strstr(attr, "TurboPrint") == NULL ? 0 : 1);
    }
}

////////////////////////////////////////////// HW Margin
void hw_margins_ppd(const char *printer_name)
{
  const char *PPDFile = cupsGetPPD (printer_name);
  ppd_file_t *ppd = ppdOpenFile(PPDFile);

  if (ppd)
    {
      ppd_attr_t *attr = ppdFindAttr(ppd, "HWMargins", NULL);

      if (attr)
        {
          float hw_top, hw_bottom, hw_left, hw_right;
          sscanf(attr->value, "%lf %lf %lf %lf", &hw_left, &hw_bottom, &hw_right, &hw_top);

          printf ("PPD: hw margin-left   %2.4f\n", hw_left);
          printf ("PPD: hw margin-right  %2.4f\n", hw_right);
          printf ("PPD: hw margin-top    %2.4f\n", hw_top);
          printf ("PPD: hw margin-bottom %2.4f\n", hw_bottom);
        }
    }

  ppdClose(ppd);
  unlink(PPDFile);
}
void hw_margins(cups_dest_t *dest, http_t *hcon, cups_dinfo_t *info)
{
  // only works with getDestWithURI()
  char *name[4] = { "left", "right", "top", "bottom" };

  for (int k=0; k<4; k++)
    {
      char opt[100];
      sprintf(opt, "media-%s-margin", name[k]);

      if (cupsCheckDestSupported(hcon, dest, info, opt, NULL))
        {
          ipp_attribute_t *attr = cupsFindDestSupported(hcon, dest, info, opt);

          if (attr)
            {
              int count = ippGetCount(attr);
              switch (ippGetValueTag(attr))
                {
                case IPP_TAG_INTEGER :
                  printf("IPP: hw margin-%s'   '%2.4f'\n", name[k], ippGetInteger(attr, 0) / 100.0); // hw margins
                  break;

                default:
                  printf("type: %d\n", ippGetValueTag(attr));
                }
            }
        }
    }
}

////////////////////////////////////////////// DEFAULT RESOLUTION

void resolution_ppd(const char *printer_name)
{
  const char *PPDFile = cupsGetPPD (printer_name);
  ppd_file_t *ppd = ppdOpenFile(PPDFile);
  int resolution = 0;

  if (ppd)
    {
      ppd_attr_t *attr = ppdFindAttr(ppd, "DefaultResolution", NULL);

      if (attr)
        {
          char *x = strstr(attr->value, "x");

          if (x)
            sscanf (x+1, "%ddpi", &resolution);
          else
            sscanf (attr->value, "%ddpi", &resolution);
        }
      else
        resolution = 300;

      while(resolution>360)
        resolution /= 2.0;

      printf("PPD: resolution %d\n", resolution);

      ppdClose(ppd);
      unlink(PPDFile);
    }
}

void resolution(cups_dest_t *dest, http_t *hcon, cups_dinfo_t *info)
{
  const char *opt = "printer-resolution";

  ////// this section is not working, dunno why!!!!
  static const char * const requested_attrs[] =
    {
      "printer-description",
      "document-format-supported",
      "color-supported",
      "pages-per-minute",
      "pages-per-minute-color",
      "media-supported",
      "media-ready",
      "media-default",
      "media-type-supported",
      "media-source-supported",
      "media-col-database",
      "sides-supported",
      "sides-default",
      "output-bin-supported",
      "output-bin-default",
      "finishings-supported",
      "finishings-default",
      "print-color-mode-supported",
      "print-color-mode-default",
      "output-mode-supported",
      "output-mode-default",
      "print-quality-supported",
      "print-quality-default",
      "printer-resolution-supported",
      "printer-resolution-default",
      "copies-supported",
      "copies-default",
      "all"
    };

  ipp_t *request = ippNewRequest(IPP_OP_GET_PRINTER_ATTRIBUTES);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI,
               "printer-uri", NULL, printer_uri);
  ippAddStrings(request, IPP_TAG_OPERATION, IPP_TAG_NAME,
                "requested-attributes",
                sizeof(requested_attrs) / sizeof(requested_attrs[0]),
                NULL, requested_attrs);

  ipp_t *response = cupsDoRequest(hcon, request, "/printers/Lexmark-X950");
  ipp_attribute_t *resolution = ippFindAttribute(response, "printer-resolution", IPP_TAG_INTEGER);
  printf("printer-resolution=%d (count:%d)\n", ippGetInteger(resolution, 0), ippGetCount(resolution));


  request = ippNewRequest(IPP_OP_CUPS_GET_DEFAULT);
  response = cupsDoRequest(hcon, request, "/");
  ipp_attribute_t *attr = ippFindAttribute(response, "printer-name", IPP_TAG_NAME);
  printf(">>>> %s\n", ippGetString(attr, 1, NULL));
  ///////

  if (cupsCheckDestSupported(hcon, dest, info, opt, NULL))
    {
      ipp_attribute_t *attr = cupsFindDestSupported(hcon, dest, info, opt);

      if (attr)
        {
          int count = ippGetCount(attr);
          switch (ippGetValueTag(attr))
            {
            case IPP_TAG_INTEGER :
              printf("IPP: resolution   %d\n", ippGetInteger(attr, 0)); // hw margins
              break;

            case IPP_TAG_RESOLUTION :
              {
                int xres, yres;
                ipp_res_t units;
                xres = ippGetResolution(attr, 0, &yres, &units);
                while(yres>360)
                  yres /= 2.0;
                printf("IPP: resolution %d\n", yres);
              }
              break;

            default:
              printf("type: %d\n", ippGetValueTag(attr));
            }
        }
    }
}

////////////////////////////////////////////// do_1

void do_1 (void)
{
  //const char *printer_uri = "ipp://localhost:631/printers/SP-3880-TurboPrint-Luster";
  //const char *printer_name = "Epson_Stylus_Pro_3880";
  //const char *printer_name = "Epson3880";
  const char *printer_name = "PDF";
  //const char *printer_name = "SP-3880-TurboPrint-Luster";
  //const char *printer_name = "HP_ENVY_7640_series_lancelot";
  //const char *printer_name = "HP_ENVY_7640_series_7BC99B_";
  //const char *printer_name = "Virtual_PDF_Printer";

  //const char *printer_name = "Lexmark-X950";

  snprintf(printer_uri, sizeof(printer_uri), "ipp://localhost:631/printers/%s", printer_name);;

  {
    cups_dest_t *dests;
    int num_dests = cupsGetDests(&dests);
    cups_dest_t *dest = cupsGetDest(printer_name, NULL, num_dests, dests);

    // to be able to list media-type when CUPS_DEST_FLAGS_DEVICE is used we need to get dest with an URI
    //cups_dest_t *dest = cupsGetDestWithURI(NULL, printer_uri);

    // printer-uri-supported is not null when use cupsGetDest()
    const char *uri = cupsGetOption("printer-uri-supported", dest->num_options, dest->options);
    printf("**** PRINTER [%s]    URI:[%s]\n", printer_name, uri);

    dest = cupsGetDestWithURI(NULL, uri);

    if (dest)
    {
      http_t *hcon = cupsConnectDest(dest, CUPS_DEST_FLAGS_DEVICE/*NONE|DEVICE*/, 30000, NULL, NULL, 0, NULL, NULL);

      if (hcon)
      {
        cups_dinfo_t *info = cupsCopyDestInfo (hcon, dest);

        is_turboprint_ppd(printer_name); printf("\n");
        //is_turboprint(dest, hcon, info); printf("\n");

        hw_margins_ppd(printer_name); printf("\n");
        //hw_margins(dest, hcon, info); printf("\n");

        resolution_ppd(printer_name); printf("\n");
        //resolution(dest, hcon, info); printf("\n");

        media_type_ppd(printer_name); printf("\n");
        //media_type(dest, hcon, info); printf("\n");

        cupsFreeDestInfo(info);
        httpClose(hcon);
      }
      else
        printf("hcon is null\n");
    }
    else
      printf("dest is null\n");

    cupsFreeDests(num_dests, dests);
  }
}

////////////////////////////////////////////// do_2

void do_attr(http_t *http, cups_dest_t *dest, cups_dinfo_t *dinfo, const char *option)
{
  ipp_attribute_t *attr = cupsFindDestSupported(http, dest, dinfo, option);

  if (attr)
    {
      if (strcmp(option, "printer-resolution")==0)
        {
          int xres, yres;
          ipp_res_t units;
          xres = ippGetResolution(attr, 0, &yres, &units);
          while(yres>360)
            yres /= 2.0;
          printf("IPP: resolution   %d\n", yres); // hw margins
        }
#if 0
      // media: A4, B0... but without the sizes/borders, better getDestMediaCount() see do_2
      else if (strcmp(option, "media")==0)
        {
          // paper sizes
          const int count = ippGetCount(attr);
          printf("option %s %d\n", option, count);
          for (int i = 0; i < count; i ++)
            {
              const char *lvalue = cupsLocalizeDestValue(http, dest, dinfo, option, ippGetString(attr, i, NULL));
              printf("  %s | %s\n", ippGetString(attr, i, NULL), lvalue?lvalue:"not localized");
            }
        }
#endif
      else if (strcmp(option, "media-col")==0)
        {
          // paper sizes
          const int count = ippGetCount(attr);
          printf("media-col %d\n", count);
          for (int i = 0; i < count; i ++)
          {
            printf("   %s\n", ippGetString(attr, i, NULL));
            do_attr(http, dest, dinfo, ippGetString(attr, i, NULL));
          }
        }
      else if (strcmp(option, "media-size")==0)
        {
          // paper sizes
          const int count = ippGetCount(attr);
          printf("media-size %d\n", count);
          for (int i = 0; i < count; i ++)
            do_attr(http, dest, dinfo, ippGetString(attr, i, NULL));
        }
      else if (strcmp(option, "document-format")==0)
        {
          const int count = ippGetCount(attr);
          printf("document-format %d\n", count);
          for (int i = 0; i < count; i ++)
          {
            printf("   format: %s\n", ippGetString(attr, i, NULL));
            do_attr(http, dest, dinfo, ippGetString(attr, i, NULL));
          }
        }
      else if (strcmp(option, "media-type")==0)
        {
          const int count = ippGetCount(attr);
          printf("media-type %d\n", count);
          for (int i = 0; i < count; i ++)
          {
            printf("   format: %s\n", ippGetString(attr, i, NULL));
          }
        }
      else if (strcmp(option, "job-sheets")==0)
        {
          const int count = ippGetCount(attr);
          printf("job-sheets %d\n", count);
          for (int i = 0; i < count; i ++)
          {
            printf("   qual: %s\n", ippGetString(attr, i, NULL));
          }
        }
      else if (strcmp(option, "print-quality")==0)
        {
          const int count = ippGetCount(attr);
          printf("print-quality %d\n", count);
          for (int i = 0; i < count; i ++)
          {
            printf("   print-quality: %d\n", ippGetInteger(attr, i));
          }
        }
      else if (strcmp(option, "media-bottom-margin")==0)
        {
          /*  Each media-x-margin value is a non-negative integer in hundredths of millimeters or
              1/2540th of an inch and specifies a hardware margin supported by the Printer.
          */
          const int count = ippGetCount(attr);
          printf("IPP: hw margin-bottom   %d %f\n", count, ippGetInteger(attr, 0) / 100.0);
        }
      else if (strcmp(option, "media-top-margin")==0)
        {
          printf("IPP: hw margin-top   %f\n", ippGetInteger(attr, 0) / 100.0);
        }
      else if (strcmp(option, "media-left-margin")==0)
        {
          printf("IPP: hw margin-left   %f\n", ippGetInteger(attr, 0) / 100.0);
        }
      else if (strcmp(option, "media-right-margin")==0)
        {
          printf("IPP: hw margin-right   %f\n", ippGetInteger(attr, 0) / 100.0);
        }
      else
        {
          printf("ignored option: %s\n", option);
        }
    }
}

void do_2 (void)
{
  //const char *printer_name = "Lexmark-X950";
  //const char *printer_name = "Epson3880";
  const char *printer_name = "PDF";
  //const char *printer_name = "Epson_Stylus_Pro_3880";
  //const char *printer_name = "SP-3880-TurboPrint-Luster";

  cups_dest_t *dests;
  int num_dests = cupsGetDests(&dests);
  cups_dest_t *dest = cupsGetDest(printer_name, NULL, num_dests, dests);

  if (!dest) return;
  // check if turboprint
#if 1
  for (int j = 0; j < dest->num_options; j++)
    printf ("     %d: %s = %s\n",
            j,
            dest->options[j].name?dest->options[j].name:"<>",
            dest->options[j].value?dest->options[j].value:"<>");
#endif

  // then loop over all attributes
  const char *uri = cupsGetOption("printer-uri-supported", dest->num_options, dest->options);
  // const char *uri = "ipp://localhost:631/printers/Epson3880";

  printf("**** PRINTER [%s]    URI:[%s]\n", printer_name, uri);

  int num_options = 0;
  cups_option_t *options = NULL;

  const char *is_turboprint1 = cupsGetOption("zedoPrinterDriver", dest->num_options, dest->options);
  const char *is_turboprint2 = cupsGetOption("printer-make-and-model", dest->num_options, dest->options);
  printf("is_turboprint : %s:%s   %s:%s\n",
         is_turboprint1, is_turboprint1?"yes":"no", is_turboprint2, strstr(is_turboprint2,"TurboPrint")?"yes":"no");

  dest = cupsGetDestWithURI(NULL, uri);

  if (dest)
    {
      http_t *http = cupsConnectDest(dest, CUPS_DEST_FLAGS_DEVICE/*NONE|DEVICE*/, 30000, NULL, NULL, 0, NULL, NULL);

      if (http)
        {
          cups_dinfo_t *dinfo = cupsCopyDestInfo (http, dest);

          ipp_attribute_t *attr = cupsFindDestSupported(http, dest, dinfo, "job-creation-attributes");
//          ipp_attribute_t *attr = cupsFindDestSupported(http, dest, dinfo, "printer-info");

          if (attr)
            {
              const int count = ippGetCount(attr);
              printf("count: %d\n", count);
//              do_attr(http, dest, dinfo, "document-format");
//              do_attr(http, dest, dinfo, "media-type");
              for (int i = 0; i < count; i ++)
                do_attr(http, dest, dinfo, ippGetString(attr, i, NULL));
            }

          return;

          // on the following code we do not get the borderless papers?
          // size.{bottom,left,right,top} are the margins for the paper, seems like a better option
          // to get the hw margins as it seems this can be different based on paper size.
          // fact is that all values are equal on my testing.

          cups_size_t size;
          const int count = cupsGetDestMediaCount(http, dest, dinfo, CUPS_MEDIA_FLAGS_DEFAULT);

          for (int k=0; k<count; k++)
          {
            if (cupsGetDestMediaByIndex(http, dest, dinfo, k, CUPS_MEDIA_FLAGS_DEFAULT, &size))
            {
              if (size.width!=0 && size.length!=0)
              {
                pwg_media_t *med = pwgMediaForPWG (size.media);
                char common_name[1000] = { 0 };

                if (med->ppd)
                  strncpy(common_name, med->ppd, sizeof(common_name));
                else
                  strncpy(common_name, size.media, sizeof(common_name));

                printf("new media paper %4d %6.2f x %6.2f (%s) (%s) bot:%.2f left:%.2f right:%.2f top:%.2f\n",
                       k, size.width / 100.0, size.length / 100.0, size.media, common_name,
                       size.bottom / 100. , size.left /100., size.right/100., size.top/100.);
              }
            }
          }
        }
    }
}

////////////////////////////////////////////// main


/// ???? what is the best way to check for borderless support?

void main (void)
{
  do_2();
}
