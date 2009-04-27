/* 
 * The Biomechanical ToolKit
 * Copyright (c) 2009, Arnaud Barré
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *     * Redistributions of source code must retain the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *     * Neither the name(s) of the copyright holders nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "btkMEXObjectHandle.h"
#include "btkMEXAdaptMeasures.h"

#include <btkAcquisition.h>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	if(nrhs!=1)
		mexErrMsgTxt("One input required.");
	if (nlhs > 2)
	 mexErrMsgTxt("Too many output arguments.");

	// First output
	btk::Acquisition::Pointer acq = btk_MOH_get_object<btk::Acquisition>(prhs[0]);
	btk::MarkerCollection::Pointer markers;
	btk::MetaDataEntry::Iterator itPoint = acq->GetMetaData()->Find("POINT");
	if (itPoint != acq->GetMetaData()->End())
	{
	  btk::MarkerCollection::Pointer allMarkers = acq->GetMarkers();
		const char* names[] = {"ANGLES", "FORCES", "MOMENTS", "POWERS", "SCALARS", "REACTIONS"};
		int numberOfNames =  sizeof(names) / (sizeof(char) * 4);
  	std::vector<std::string> generalBlackList, blackList;
		for(int i = 0 ; i < numberOfNames ; ++i)
		{
			btk::MetaDataEntry::CollapseChildrenValues(blackList, *itPoint, names[i]);
		  generalBlackList.insert(generalBlackList.end(), blackList.begin(), blackList.end());
		}

		if (generalBlackList.empty())
			markers = allMarkers;
		else
		{
			markers = btk::MarkerCollection::New();
			for(btk::MarkerCollection::Iterator it3DPoint = allMarkers->Begin() ; it3DPoint != allMarkers->End() ; ++it3DPoint)
			{
			  std::vector<std::string>::iterator itLabel = generalBlackList.begin();
				bool labelFound = false;
				while(itLabel != generalBlackList.end())
				{
					if ((*it3DPoint)->GetLabel().compare(*itLabel) == 0)
					{
						itLabel = generalBlackList.erase(itLabel);
						labelFound = true;
						break;
					}
					++itLabel;
				}
				if (!labelFound)
					markers->InsertItem(*it3DPoint);
				if (generalBlackList.empty())
					break;
			}
		}
	}
	else
		markers = acq->GetMarkers();

	char** fieldnames = 0;
	plhs[0] = btkMEXAdaptMeasures<btk::Marker>(markers, &fieldnames);
	int numberOfMarkers = acq->GetMarkerNumber();
	for (int i = 0 ; i < markers->GetItemNumber() ; ++i)
		delete[] fieldnames[i];
	delete[] fieldnames;

  // Second output (optionnal)
	if (nlhs == 2)
	{
    const char* info[] = {"frequency", "units"};
		int numberOfFields =  sizeof(info) / (sizeof(char) * 4);
		char* units = 0;
    btk::MetaDataEntry::Pointer metadata = acq->GetMetaData();
    btk::MetaDataEntry::ConstIterator itPoint = metadata->Find("POINT");
		if ((itPoint != metadata->End()) && (numberOfMarkers != 0))
		{
			btk::MetaDataEntry::ConstIterator itUnit = (*itPoint)->Find("UNITS");
			if (itUnit != (*itPoint)->End())
			{
				btk::MetaDataEntryValue::ConstPointer pointUnit = (*itUnit)->GetMetaDataEntryValue();
				// Erase blank spaces at the beginning and the end of the string.
				std::string str = pointUnit->GetValue(0);
        str = str.erase(str.find_last_not_of(' ') + 1);
        str = str.erase(0, str.find_first_not_of(' '));
				units = new char[str.length() + 1];
        strcpy(units, str.c_str());
			}
		}
		if (units == 0)
		{
			//mexWarnMsgTxt("No POINT:UNITS parameter. The makers' unit is empty.");
			units = new char[1];
			units[0] = '\0';
		}
		plhs[1] = mxCreateStructMatrix(1, 1, numberOfFields, info);
		mxArray* firstFrame  = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
    mxArray* frequency = mxCreateDoubleMatrix(1, 1, mxREAL);
    *mxGetPr(frequency) = acq->GetMarkerFrequency();
		const char* ALL[] = {"ALLMARKERS"};
		mxArray* unitsStruct = mxCreateStructMatrix(1, 1, 1, ALL);
		mxSetFieldByNumber(unitsStruct, 0, 0, mxCreateString((const char*)units));
		mxSetFieldByNumber(plhs[1], 0, 0, frequency);
		mxSetFieldByNumber(plhs[1], 0, 1, unitsStruct);

		delete[] units;
	}
};
