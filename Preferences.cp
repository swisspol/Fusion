#include			"Structures.h"
#include			"Editor Header.h"

//ROUTINES:

static OSErr ScanForFolder(short volumeID, long parentFolderID, Str63 folderName, long* folderID)
{
	short			idx;
	CInfoPBRec		cipbr;
	HFileInfo		*fpb = (HFileInfo*) &cipbr;
	DirInfo			*dpb = (DirInfo*) &cipbr;
	
	fpb->ioVRefNum = volumeID;
	fpb->ioNamePtr = theString;
	for(idx = 1; true; ++idx) {
		fpb->ioDirID = parentFolderID;
		fpb->ioFDirIndex = idx;
		if(PBGetCatInfo(&cipbr, false))
		break;
		
		if(fpb->ioFlAttrib & 16 && EqualString(theString, folderName, false, false)) {
			*folderID = dpb->ioDrDirID;
			return noErr;
		}
	}
	
	return -1; //Folder not found
}

static OSErr Find_FrenchTouchPreferencesFolder(short* volumeID, long* folderNum)
{
	OSErr		theError;
	Str63		folderName;
	long		prefFolderNum;
	
	theError = FindFolder(kOnSystemDisk, 'pref', kDontCreateFolder, volumeID, &prefFolderNum);
	if(theError)
	return theError;
	
	GetIndString(folderName, 128, 1);
	theError = ScanForFolder(*volumeID, prefFolderNum, folderName, folderNum);
	if(theError)
	return theError;
	
	return noErr;
}

void Preference_Read()
{
	OSErr		theError;
	short		fileID,
				volumeID;
	long		bytesNumber,
				prefFolderNum;
	Str63		prefName;
	
	theError = Find_FrenchTouchPreferencesFolder(&volumeID, &prefFolderNum);
	if(theError)
	goto New;
	
	GetIndString(prefName, 128, 2);
	theError = HOpen(volumeID, prefFolderNum, prefName, fsRdPerm, &fileID);
	if(theError)
	goto New;
	SetFPos(fileID, 1, 0);
	bytesNumber = sizeof(Preferences);
	theError = FSRead(fileID, &bytesNumber, &thePrefs);
	FSClose(fileID);
	
	if(thePrefs.version != kPref_Version)
	goto New;
	
	return;
	
	New:
	thePrefs.version = kPref_Version;
	thePrefs.flags = kPref_FlagFiltering;
	thePrefs.ambient = 0.5;
	thePrefs.local = 1.0;
	thePrefs.raveEngineID = -1;
	thePrefs.windowPosition[0].h = 660;
	thePrefs.windowPosition[0].v = 40;
	thePrefs.windowPosition[1].h = 640;
	thePrefs.windowPosition[1].v = 290;
	thePrefs.windowPosition[2].h = 640;
	thePrefs.windowPosition[2].v = 40;
	thePrefs.windowPosition[3].h = 50;
	thePrefs.windowPosition[3].v = 480;
	thePrefs.windowPosition[4].h = 40;
	thePrefs.windowPosition[4].v = 470;
	thePrefs.backColorRed = 0.0;
	thePrefs.backColorGreen = 0.0;
	thePrefs.backColorBlue = 0.0;
}

void Preference_Write()
{
	OSErr		theError;
	short		fileID,
				volumeID;
	long		bytesNumber,
				prefFolderNum;
	Str63		prefName,
				folderName;
	
	theError = Find_FrenchTouchPreferencesFolder(&volumeID, &prefFolderNum);
	if(theError) {
		theError = FindFolder(kOnSystemDisk, 'pref', kDontCreateFolder, &volumeID, &prefFolderNum);
		if(theError)
		return;
		GetIndString(folderName, 128, 1);
		DirCreate(volumeID, prefFolderNum, folderName, &prefFolderNum);
	}
	
	GetIndString(prefName, 128, 2);
	theError = HOpen(volumeID, prefFolderNum, prefName, fsRdWrPerm, &fileID);
	if(theError) {
		theError = HCreate(volumeID, prefFolderNum, prefName, k3DFileCreator, 'pref');
		if(theError)
		return;
		theError = HOpen(volumeID, prefFolderNum, prefName, fsRdWrPerm, &fileID);
		if(theError)
		return;
	}
	
	SetFPos(fileID, 1, 0);
	bytesNumber = sizeof(Preferences);
	theError = FSWrite(fileID, &bytesNumber, &thePrefs);
	FSClose(fileID);
}