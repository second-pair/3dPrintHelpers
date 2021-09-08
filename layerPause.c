/*  *--<Preface>--*  //

 -=-  Author Details  -=-
 Blair Edwards
 Personal time.

 -=-  Dates  -=-
 Started 2021-09-06

 -=-  Description  -=-
 This programme attempts to insert a pause into Marlin-flavoured G-Code, after a specified layer's code.  The print should remain paused until the operator uses the LCD to resume the print.
 Input is by keyboard using STDIN.
 Output is text through STDOUT and through file streams.

 -=-  Task  -=-
 -=>  Get input from the user - which file to edit and before which layer to pause at.
 -=>  Seek through the file and work out where this layer starts.
 -=>  Insert the code block just before this layer.

 -=-  Notes  -=-
 -=>  I've developed my own commenting notation for things that "aren't done" one way or antoher.  Such as:
	 -  //#  TODO
	 -  //?  Not sure / query
	 -  //!  Important note / relevant as technology advances
 -=>  The idea here is the operator can then interact with the machine, then use the inbuilt display to resume the print.
 -=>  I made this primarily for embedding magnets and swapping filament at specified layers.

 -=-  TODO  -=-
 -=>  Have the comment prefix be changeable.
 -=>  Allow specifying multiple layers.

//  *--</Preface>--*  */



//  *--<Preparations>--*  //

//  Includes
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

//  Defines
#define BUF_SIZE UINT8_MAX
//  My generategd G-Code had CR+LF, so I guess I'd better respect it...
#define INSERT_TEXT "\n\n;  Perform magnet insert procedure.\nG91  ;  Relative mode.\nG1 Y30 Z10 E-12  ;  Move away.\nM0 Insert Magnet  ;  Wait for LCD button press.\nG1 Y-30 Z-10 E12  ;  Move back.\nG90  ;  Absolute mode.\n\n"
#define INSERT_TEXT_LENGTH 223

//  Prototype Functions
int main (void);

//  Global Variables

//  *--</Preparations>--*  //



//  *--<Macros>--*  //

#ifndef LOG_LEVEL
	#define LOG_LEVEL 0
#endif
//  Log to STDOUT, provided we meet the verbosity requirements.
#define _LOG(logLevel, logString, ...) \
({ \
	if (logLevel <= LOG_LEVEL) \
		printf ("LOG%d:  %s ():  " logString, logLevel, __FUNCTION__, ##__VA_ARGS__); \
})
//  Check if a variable is NULL, `return retCode;` if it does.
#define _NULL_RETURN(variable, retCode) \
({ \
	if (variable == NULL) \
	{ \
		_LOG (0, ""#variable" == NULL!  Returning..."); \
		return retCode; \
	} \
})
//  As above, but only perform a carriage return, not a line feed.
//  To allow for more verbose logging within an"_LOG_STREAM block", we ony do this for the same 'logLevel' as our current verbosity - else we just perform a normal `_LOG ()`.
#define _LOG_STREAM(logLevel, logString, ...) \
({ \
	if (logLevel == LOG_LEVEL) \
	{ \
		printf ("LOG%d:  %s ():  " logString "\r", logLevel, __FUNCTION__, ##__VA_ARGS__);  fflush (stdout); \
	} \
	else \
		_LOG (logLevel + 1, logString "\n", ##__VA_ARGS__); \
})
//  Close out a previous `_LOG_STREAM ()` with a carriage return, if appropriate.
#define _LOG_STREAM_END(logLevel) \
({ \
	if (logLevel == LOG_LEVEL) \
		putchar ('\n'); \
})

//  *--</Macros>--*  //



//  *--<Main Code>--*  //

int main (void)
{
	printf ("-=-  Layer Pause Inserter Programme  -=-\n");

	//  Get input from the user - which file to edit and before which layer to pause at.
	printf ("Please tell me which file you would like to modify:  ");
	char fileName [BUF_SIZE];
	fgets (fileName, BUF_SIZE, stdin);
	_NULL_RETURN (fileName, 1);
	_LOG (2, "Stripping quote characters from the file name.\n");
	for (uint8_t i = 0; i < BUF_SIZE; i ++)
	{
		_LOG_STREAM (4, "%u/%d:  %c", i, BUF_SIZE, fileName [i]);
		//  Strip quote characters.
		while (fileName [i] == '"' || fileName [i] == '\'')
		{
			for (uint8_t j = i; j < BUF_SIZE; j ++)
				fileName [j] = fileName [j + 1];
		}
		//  Terminate at terminating etc characters.
		if (fileName [i] == '\0' || fileName [i] == '\r' || fileName [i] == '\n' || fileName [i] == EOF)
		{
			fileName [i] = '\0';
			break;
		}
	}
	_LOG_STREAM_END (4);

	//  Rename the file.
	char fileNameBak [BUF_SIZE];
	snprintf (fileNameBak, BUF_SIZE, "%s.bak", fileName);
	_LOG (3, "Renaming file %s -> %s,\n", fileName, fileNameBak);
	if (rename (fileName, fileNameBak))
	{
		_LOG (0, "Could not rename the file!\n");
		perror ("rename");
		return 1;
	}

	//  Open the file.  We use binary mode so we treat '\r' as its own character, if it's present.
	// ~/homeDir/Sync/Documents/Techie\ Stuff/CAD/3D\ Printing/_GCode/wifiQrCode_blairware-withMagnets_CE3v2-0.12Speedy_pla-3dqf.gcode
	// ~/homeDir/Sync/Documents/Techie Stuff/CAD/3D Printing/_GCode/wifiQrCode_blairware-withMagnets_CE3v2-0.12Speedy_pla-3dqf.gcode
	// /mnt/c/Users/blair/homeDir/Sync/Documents/Techie Stuff/CAD/3D Printing/_GCode/wifiQrCode_blairware-withMagnets_CE3v2-0.12Speedy_pla-3dqf.gcode
	// C:/Users/blair/homeDir/Sync/Documents/Techie Stuff/CAD/3D Printing/_GCode/wifiQrCode_blairware-withMagnets_CE3v2-0.12Speedy_pla-3dqf.gcode
	// C:\\Users\\blair\\homeDir\\Sync\\Documents\\Techie Stuff\\CAD\\3D Printing\\_GCode\\wifiQrCode_blairware-withMagnets_CE3v2-0.12Speedy_pla-3dqf.gcode
	_LOG (3, "Opening moved file.\n");
	FILE* theFile = fopen (fileName, "wb");
	if (theFile == NULL)
	{
		perror ("fopen");
		_NULL_RETURN (theFile, 1);
	}
	_LOG (3, "Opening new destination file.\n");
	FILE* theFileBak = fopen (fileNameBak, "rb");
	if (theFileBak == NULL)
	{
		perror ("fopen");
		_NULL_RETURN (theFileBak, 1);
	}

	printf ("And which layer would you like to pause after?  (0 for before the first one.)  ");
	char layerNumber_string [BUF_SIZE];
	fgets (layerNumber_string, BUF_SIZE, stdin);
	_NULL_RETURN (layerNumber_string, 1);
	uintmax_t layerNumber = strtol (layerNumber_string, NULL, 10);
	_LOG (2, "Aiming for layer %ju.\n", layerNumber);

	//  Debug
	//printf ("Layer %ju, file %s.\n", layerNumber, fileName);

	//  Seek through the file and work out where the layer comments start.
	//  ;LAYER:17
	_LOG (3, "Seeking for comments.\n");
	while (1)
	{
		//  Loop until we get to a new comment.
		while (fgetc (theFileBak) != ';')
		{
			if (feof (theFileBak))
			{
				_LOG (0, "Could not find that layer - reached EOF!\n");
				exit (1);
			}
		}
		//  Read the subsequent data.
		char theComment [BUF_SIZE];
		fgets (theComment, BUF_SIZE, theFileBak);
		_LOG (5, "Comment:  %10s\n", theComment);
		_LOG (4, "Checking comment validity - position %ld.\n", ftell (theFileBak));
		uint8_t length = 0;
		while (theComment [length] != EOF && theComment [length] != '\0' && length < BUF_SIZE - 1)
			length ++;
		//  Check there's enough data in the file.
		if (length > 6)
		{
			//  Check if it's a layer comment.
			if (theComment [0] == 'L' && theComment [1] == 'A' && theComment [2] == 'Y' && theComment [3] == 'E' && theComment [4] == 'R' && theComment [5] == ':')
			{
				_LOG (3, "Valid ';LAYER:' comment - checking layer number.\n");
				//  Readahead to find the end of the number.
				uint8_t numberLength = 0;
				while (theComment [numberLength + 6] >= '0' && theComment [numberLength + 6] <= '9')
					numberLength ++;
				theComment [numberLength + 6] = '\0';
				//  Now extract the number.
				uint8_t currNumber = strtol ((char*)&theComment + 6, NULL, 10);

				//  Is this the corect layer?
				if (currNumber == layerNumber)
				{
					_LOG (2, "Layer %u found - moving on!\n", currNumber);
					//  Yes!  Break from the loop and continue onwards.
					break;
				}
				_LOG (3, "Layer %u is not what we're after - seeking the next comment.\n", currNumber);
			}
		}
	}

	//  Desired:  467970
	//  Actual :  486004
    //  Insert the code block just before this layer.
	//  Start by copying the characters up to this point from the backup file to the new file.
	intmax_t insertPos = ftell (theFileBak) - 2;
	if (insertPos < 0)
	{
		_LOG (0, "`ftell ()` failed!\n");
		return 1;
	}
	_LOG (2, "File position %jd.  Rewinding.\n", insertPos);
	rewind (theFileBak);
	_LOG (2, "Copying bytes preceding the comment.\n");
	while (insertPos --)
		fputc (fgetc (theFileBak), theFile);
	_LOG (2, "Inserting the comment.\n");
	fprintf (theFile, "%s", INSERT_TEXT);
	_LOG (2, "Copying the rest of the file.\n");
	char curr;
	while (1)
	{
		curr = fgetc (theFileBak);
		if (feof (theFileBak))
		{
			_LOG (3, "EOF reached.\n");
			break;
		}
		fputc (curr, theFile);
	}

	_LOG (2, "Donezo!  Cleaning up and heading on out.\n");
	fclose (theFile);
	fclose (theFileBak);
	return 0;
}

//  *--</Main Code>--*  //
