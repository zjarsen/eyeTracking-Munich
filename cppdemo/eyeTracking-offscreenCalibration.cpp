#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "stdafx.h"
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "iViewXAPI.h"
//added
#include <string.h>
#include <string>
#define PORTNUM 12355
#define BUFMAX 1024
//end

int __stdcall SampleCallbackFunction(SampleStruct sampleData)
{
	std::cout << "From Sample Callback X: " << sampleData.leftEye.gazeX << " Y: " << sampleData.leftEye.gazeY << std::endl;
	return 1;
}

int _tmain(int argc, _TCHAR* argv[])
{
	AccuracyStruct accuracyData; 
	SystemInfoStruct systemInfoData; 
	CalibrationStruct calibrationData;
	CalibrationPointStruct calibrationPointStruct;
	SampleStruct sampleData;
	SpeedModeStruct speedModeData;
	int returnedValue = 0;
	int numOfCalibrationPoints;

	//add
	SOCKET sock;
	sockaddr_in serverAddr;
	WSADATA wsaDat;

	int wsaError = WSAStartup(MAKEWORD(2, 2), &wsaDat);

	if (!wsaError)
	{
		sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if (sock == INVALID_SOCKET)
		{
			wprintf(L"socket function failed with error = %d\n", WSAGetLastError());
			exit(-1);
		}

		ZeroMemory(&serverAddr, sizeof(serverAddr));

		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(PORTNUM);
		//change the address below to your connected ROS computer address
		serverAddr.sin_addr.s_addr = inet_addr("192.168.15.49");

		sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		//windowsEchoLoop(sock, (SOCKADDR*)& serverAddr, sizeof(serverAddr));
		
		
	}
	else
	{
		return EXIT_FAILURE;
	}
	//end

	int ret_calibrate = 0, ret_validate = 0, ret_connect = 0, ret_acceptCalibrationPoint; 
	char c = ' ';
	char repeat = ' ';
	int isSuccessful = 0;
	char r = ' ';

	std::cout << "Output of off-screen calibration"<< std::endl;

	// connect to iViewX 
	ret_connect = iV_Connect("127.0.0.1", 4444, "127.0.0.1", 5555);

	switch(ret_connect)
	{
		case RET_SUCCESS:
			std::cout <<  "Connection was established successfully" << std::endl;
			break;
		case ERR_COULD_NOT_CONNECT:
			std::cout <<  "Connection could not be established" << std::endl;
			break;
		case ERR_WRONG_PARAMETER:
			std::cout <<  "Wrong Parameter used" << std::endl;
			break;
		default:
			std::cout <<  "Any other error appeared" << std::endl;
			return 0;
	}

	if(ret_connect == RET_SUCCESS)
	{
		// Ready to perform a calibration?
 		std::cout <<  "Press Enter to calibrate" << std::endl;
		getchar(); 
 
 		if(1)
		{
			// set up calibration point
			iV_SetSpeedMode(30);
			iV_ChangeCalibrationPoint(1, 30, 30);

			// set up calibration
			calibrationData.method = 5;
			calibrationData.speed = 0;
			calibrationData.displayDevice = 0;
			calibrationData.targetShape = 2;
			calibrationData.foregroundBrightness = 250;
			calibrationData.backgroundBrightness = 230;
			calibrationData.autoAccept = 2;
			calibrationData.targetSize = 20;
			calibrationData.visualization = 1;
			strcpy_s(calibrationData.targetFilename, 256, "");

			iV_SetupCalibration(&calibrationData);

			//loop for calibration (successful calibration can skip the loop)
			while(isSuccessful == 0){
				
				// start calibration
				ret_calibrate = iV_Calibrate();

				switch (ret_calibrate)
				{
				case RET_SUCCESS:
					std::cout << "Validation starts" << std::endl;

					// start validation 
					ret_validate = iV_Validate();

					// show accuracy only if validation was successful
					if (ret_validate == RET_SUCCESS)
					{
						std::cout << "iV_GetAccuracy: " << iV_GetAccuracy(&accuracyData, 0) << std::endl;
						std::cout << "AccuracyData DevX: " << accuracyData.deviationLX << " DevY: " << accuracyData.deviationLY << std::endl;
						
						//the statement below is applied to the situation of successful projection point setup. When the deviation is too high, the calibration fails. make it a comment when the setup is not ready. conact Andy using zhuj0017@e.ntu.edu.sg for help
						//if ((accuracyData.deviationLX <= 0.8) && (accuracyData.deviationLY) <= 0.8) {
						if(1){
							isSuccessful = 1;
							std::cout << "Calibration is successful. Press Enter to start tracking" << std::endl;
						}
						else {
							std::cout << "the accuracy is too low ";
						}
						getchar();
					}
					break;
				case ERR_NOT_CONNECTED:
					std::cout << "iViewX is not reachable" << std::endl;
					break;
				case ERR_WRONG_PARAMETER:
					std::cout << "Wrong Parameter used" << std::endl;
					break;
				case ERR_WRONG_DEVICE:
					std::cout << "Not possible to calibrate connected Eye Tracking System" << std::endl;
					break;
				default:
					std::cout << "Calibration quited" << std::endl;
					r = 'q';
					getchar();
					break;
				}

				if (r == 'q') {
					break;
				}

				if (isSuccessful == 0) {
					//when the calibration failed: shall we recalibrate?
					std::cout << "The calibration failed. Key in any value to recalibrate, or press q to quit the calibration." << std::endl;
					r = getchar();
					//flush the buffer
					getchar();
					//if yes, continue the loop. if no, end of the program.
					if (r == 'q') {
						break;
					} 
				}

			}
		}

		if (r == 'q') {
			return 0;
		}

		// start data output via callback function
		// define a callback function for receiving samples. When you want the eye data to be sent to ROS, comment the statement below, or the data will be shown on the NUC screen
		//iV_SetSampleCallback(SampleCallbackFunction);

		
		while (1) {
			//add windowsEchoLoop
			int bytesRead;
			int sendToResult;
			char inputBuffer[BUFMAX] = { 0 };
			char BufferL[BUFMAX] = { 0 };// + std::to_string(accuracyData.deviationLX);
			char BufferR[BUFMAX] = { 0 };
			int i;

			//call back
			iV_GetSample(&sampleData);
			sprintf(BufferL, "%lf", sampleData.leftEye.gazeX);
			sprintf(BufferR, "%lf", sampleData.leftEye.gazeY);
			strcat(inputBuffer, BufferL);
			strcat(inputBuffer, " _ ");
			strcat(inputBuffer, BufferR);

			//call back end
			printf("%s\n", inputBuffer);
			printf("\n");
			// = sendto(sock, inputBuffer, strlen(inputBuffer), 0, (SOCKADDR*)& serverAddr, sizeof(serverAddr));

			//if (sendToResult == SOCKET_ERROR) {
				//wprintf(L"sendto failed with error: %d\n", WSAGetLastError());
				//closesocket(sock);
				//WSACleanup();

				//puts("Press any key to continue");
				//getc(stdin);
				//exit(sendToResult);
				//}
			//end
		}				

		getchar();
		
		// disable callback
		iV_SetSampleCallback(NULL);

		// disconnect
		std::cout << "iV_Disconnect: " << iV_Disconnect() << std::endl;

		getchar();

	}

	return 0;
}