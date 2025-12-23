/*
   Summary.c - summary printer.
   Copyright (C) 2002 Imre Leber
*/

#include <stdio.h>
#include <string.h>
#include "..\kitten.h" /* WICHTIG: NLS Support */
#include "fte.h"

#include "fatsum.h"
#include "filessum.h"

static void PrintSummaryLine(unsigned long digits, char* kind);

BOOL ReportVolumeSummary(RDWRHandle handle)
{
  int i;
  unsigned long serialnum, clustersindataarea, totalnumberofsectors;
  struct FatSummary fatinfo;
  struct FilesSummary filesinfo;
  unsigned char sectorspercluster;
  struct DirectoryPosition volumepos;
  struct DirectoryEntry volumeentry;
  BOOL IsProblematic;

  char buffer[120]; 

  unsigned bytespercluster;

  IsProblematic = !GetFATSummary(handle, &fatinfo) ||
                  !GetFilesSummary(handle, &filesinfo);

  sectorspercluster = GetSectorsPerCluster(handle);
  if (!sectorspercluster)
  {
     printf(kittengets(4, 1, "Cannot get summary information\n"));
     return FALSE;
  }
  bytespercluster = (unsigned) sectorspercluster * BYTESPERSECTOR;

  clustersindataarea = GetClustersInDataArea(handle);
  if (!clustersindataarea)
  {
     printf(kittengets(4, 1, "Cannot get summary information\n"));
     return FALSE;
  }

  /* 1. Schritt: Volume Label (Name) prÅfen und anzeigen */
  switch (GetRootDirVolumeLabel(handle, &volumepos))
  {
     case TRUE:
          if (!GetDirectory(handle, &volumepos, &volumeentry))
          {
             printf(kittengets(4, 1, "Cannot get summary information\n"));
             return FALSE;
          }

          printf(kittengets(4, 2, "Volume label is "));

          for (i = 0; i < 8; i++)
              printf("%c", volumeentry.filename[i]);
          for (i = 0; i < 3; i++)
              printf("%c", volumeentry.extension[i]);

          /* " created on " -> " erstellt am " */
          printf(kittengets(4, 3, " created on "));

          /* TODO: make the string according to the language format */
          printf("%02d-%02d-%02d, %02d:%02d:%02d",
                 (int)volumeentry.LastWriteDate.year+1980,
                 (int)volumeentry.LastWriteDate.month,
                 (int)volumeentry.LastWriteDate.day,

                 (int)volumeentry.LastWriteTime.hours,
                 (int)volumeentry.LastWriteTime.minute,
                 (int)volumeentry.LastWriteTime.second);

          puts("");
          break;

     case FALSE:
          /* Kein Name vorhanden - macht nichts, weiter gehts */
          break;
       
     case FAIL:
          printf(kittengets(4, 1, "Cannot get summary information\n"));
          return FALSE;
  }

  /* 2. Schritt: Seriennummer IMMER anzeigen (FORCE MODE) */
  /* Wir haben den switch(IsVolumeDataFilled) entfernt. */
  /* Wir zeigen die Nummer jetzt immer an, auch wenn sie 0000-0000 ist */
  
  serialnum = GetDiskSerialNumber(handle);
  
  /* High-Word (obere 16 bit) zuerst, dann Low-Word (untere 16 bit) */
  printf(kittengets(4, 4, "The disk serial number is %04X-%04X\n"),
           (unsigned) (serialnum >> 16),
           (unsigned) (serialnum & 0xFFFF));


  puts("");

  /* Print the total number of bytes in the volume. */
  totalnumberofsectors = clustersindataarea * sectorspercluster;
  if (totalnumberofsectors >= 8388608L)
  {
     PrintSummaryLine(totalnumberofsectors / 2, /* Assuming BYTESPERSECTOR == 512 */
                      kittengets(4, 5, "Kb total drive size\n")); 
  }
  else
  {
     PrintSummaryLine(totalnumberofsectors * BYTESPERSECTOR,
                      kittengets(4, 6, "bytes total drive size\n"));
  }

  /* Print the files summary information. */
  if (filesinfo.SizeOfAllFiles[1])
  {
     sprintf(buffer, kittengets(4, 7, "Kb in a total of %lu files"), filesinfo.TotalFileCount);
     PrintSummaryLine((filesinfo.SizeOfAllFiles[1] << 22) +
                                  (filesinfo.SizeOfAllFiles[0] >> 10),
                      buffer);
  }
  else
  {
     sprintf(buffer, kittengets(4, 8, "bytes in a total of %lu files"),
             filesinfo.TotalFileCount);
             
     PrintSummaryLine(filesinfo.SizeOfAllFiles[0], buffer);
  }

  if (filesinfo.HiddenFileCount)
  {
     if (filesinfo.SizeOfHiddenFiles[1])
     {
        sprintf(buffer, kittengets(4, 9, "Kb in %lu hidden files"), filesinfo.HiddenFileCount);
        PrintSummaryLine((filesinfo.SizeOfHiddenFiles[1] << 22) +
                                    (filesinfo.SizeOfHiddenFiles[0] >> 10),
                         buffer);
     }
     else
     {
        sprintf(buffer, kittengets(4, 10, "bytes in %lu hidden files"), filesinfo.HiddenFileCount);
        PrintSummaryLine(filesinfo.SizeOfHiddenFiles[0], buffer);
     }
  }

  if (filesinfo.SystemFileCount)
  {
     if (filesinfo.SizeOfSystemFiles[1])
     {
        sprintf(buffer, kittengets(4, 11, "Kb in %lu system files"), filesinfo.SystemFileCount);
        PrintSummaryLine((filesinfo.SizeOfSystemFiles[1] << 22) +
                                   (filesinfo.SizeOfSystemFiles[0] >> 10),
                         buffer);
     }
     else
     {
        sprintf(buffer, kittengets(4, 12, "bytes in %lu system files"), filesinfo.SystemFileCount);
        PrintSummaryLine(filesinfo.SizeOfSystemFiles[0], buffer);
     }
  }

  if (filesinfo.SizeOfDirectories[1])
  {
     sprintf(buffer, kittengets(4, 13, "Kb in %lu directories"), filesinfo.DirectoryCount);
     PrintSummaryLine((filesinfo.SizeOfDirectories[1] << 22) +
                                  (filesinfo.SizeOfDirectories[0] >> 10),
                      buffer);
  }
  else
  {
     sprintf(buffer, kittengets(4, 14, "bytes in %lu directories"), filesinfo.DirectoryCount);
     PrintSummaryLine(filesinfo.SizeOfDirectories[0], buffer);
  }

  /* Print the total size of files */
  if (filesinfo.SizeOfAllFiles[1])
  {
     PrintSummaryLine((filesinfo.TotalSizeofFiles[1] << 22) +
                                  (filesinfo.TotalSizeofFiles[0] >> 10),
                      kittengets(4, 15, "Kb total size of files"));
  }
  else
  {
     PrintSummaryLine(filesinfo.TotalSizeofFiles[0],
                      kittengets(4, 16, "total size of files"));
  }

  /* Print the free space (same method as for the total drive size). */
  totalnumberofsectors = fatinfo.numoffreeclusters * sectorspercluster;
  if (totalnumberofsectors >= 8388608L)
  {
     PrintSummaryLine(totalnumberofsectors / 2,
                      kittengets(4, 17, "Kb available on the volume"));
  }
  else
  {
     PrintSummaryLine(totalnumberofsectors * BYTESPERSECTOR,
                      kittengets(4, 18, "bytes available on the volume"));
  }

  puts("");

  /* Print the FAT summary information. */
  PrintSummaryLine(bytespercluster, kittengets(4, 19, "bytes in every cluster"));
  PrintSummaryLine(fatinfo.totalnumberofclusters, kittengets(4, 20, "total number of clusters"));

  if (fatinfo.numofbadclusters)
     PrintSummaryLine(fatinfo.numofbadclusters, kittengets(4, 21, "number of bad clusters"));

  if (fatinfo.numoffreeclusters)
     PrintSummaryLine(fatinfo.numoffreeclusters, kittengets(4, 22, "number of free clusters"));

  if (IsProblematic || IsTreeIncomplete())
  {
     printf(kittengets(4, 23, "\nThere was a problem getting disk information, the summary may be wrong\n"));
  }
     
  return TRUE;
}

static void PrintSummaryLine(unsigned long digits, char* kind)
{
   int len, i;
   char buffer[33];

   /* Zahl formatieren (1.000.000) */
   if (digits < 1000)
   {
      sprintf(buffer, "%lu", digits);
   }
   if (digits >= 1000)
   {
      sprintf(buffer, "%lu.%03lu", digits / 1000, digits % 1000);
   }
   if (digits >= 1000000L)
   {
      sprintf(buffer, "%lu.%03lu.%03lu", digits / 1000000L,
                                         (digits % 1000000L) / 1000,
                                         digits % 1000);
   }

   len = strlen(buffer);

   if (len < 15)
      for (i = 0; i < (15 - len); i++)
          printf(" ");

   printf("%s ", buffer);
   printf("%s\n", kind);
}