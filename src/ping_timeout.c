/*
 * ping_timeout.c:
 *
 * (c) 2017, neolit123 [at] gmail
 * this program is released in the public domain without warranty of any kind!
 *
 * a small Windows console application that will play a sound and show
 * a message box after succeeding to ICMP echo an IPV4 address.
 *
 * the program can be aborted with CTRL+C.
*/

#include <stdio.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <signal.h>
#include <process.h>

#include <timeout_utils.h>

#define SHOW_MESSAGE  1
#define ONE_SECOND_MS 1000

static WORD consoleAttributes;
static LONG keepRunning;
static LONG timeSec;

static void signalHandler(int signal)
{
	(void)signal;
	printfColor(FCOLOR_RED, BCOLOR_NULL, "\nctrl+c triggered!\n");
	InterlockedExchange(&keepRunning, 0);
}

static void timer(void *param)
{
	(void)param;
	while (InterlockedExchangeAdd(&keepRunning, 0)) {
		InterlockedExchangeAdd(&timeSec, 1);
		Sleep(ONE_SECOND_MS);
	}
}

int main(int argc, char **argv)
{
	HANDLE hIcmpFile;
	DWORD retVal = 0;
	LPVOID replyBuffer = NULL;
	DWORD replySize = 0;
	PICMP_ECHO_REPLY echoReply;
	LONG time_sec_local = 0;

	char ip[128] = "216.58.212.46"; /* google.com */
	char buf_suc_status[128];
	char buf_suc_time[128];
	char buf_message_box[256];
	char sendData[32] = "data";
	unsigned long ipAddr = INADDR_NONE;
	unsigned int ipLen;
	int ret = 0;

	/* check IP from command line */
	if (argc > 1) {
		memset(ip, 0, sizeof(ip));
		ipLen = strlen(argv[1]);
		if (ipLen > sizeof(ip) - 1)
			ipLen = sizeof(ip) - 1;
		memcpy((void *)ip, (void *)argv[1], ipLen);
	}

	InterlockedExchange(&keepRunning, 1);
	InterlockedExchange(&timeSec, 0);
	saveConsoleAttributes(&consoleAttributes);

	_beginthread(timer, 0, NULL);

	/* setup a CTRL+C handler */
 	signal(SIGINT, signalHandler);

	printfColor(FCOLOR_YELLOW, BCOLOR_NULL, "\n[*] ping timeout!\n");

	/* validate IP */
	ipAddr = inet_addr(ip);
	if (ipAddr == INADDR_NONE) {
		printfColor(FCOLOR_RED, BCOLOR_NULL, "\n[ERROR] invalid IP address: %s\n", ip);
		getchar();
		ret = 1;
		goto exit;
	}

	/* validate IP */
	hIcmpFile = IcmpCreateFile();
	if (hIcmpFile == INVALID_HANDLE_VALUE) {
		printfColor(FCOLOR_RED, BCOLOR_NULL, "\n[ERROR] IcmpCreateFile() return error: %ld\n", GetLastError());
		getchar();
		ret = 1;
		goto exit;
	}

	/* allocate reply buffer */
	replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData);
	replyBuffer = (VOID*) malloc(replySize);
	if (replyBuffer == NULL) {
		printfColor(FCOLOR_RED, BCOLOR_NULL, "\n[ERROR] unable to malloc() %d bytes!\n", replySize);
		getchar();
		ret = 1;
		goto exit_handle;
	}

	printfColor(FCOLOR_CYAN, BCOLOR_NULL, "\nechoing %s with %d bytes of data\n\n", ip, sizeof(sendData));

	while (InterlockedExchangeAdd(&keepRunning, 0)) {
		time_sec_local = InterlockedExchangeAdd(&timeSec, 0);

		retVal = IcmpSendEcho(hIcmpFile, ipAddr, sendData, sizeof(sendData),
			NULL, replyBuffer, replySize, ONE_SECOND_MS);

		if (retVal != 0) {
			echoReply = (PICMP_ECHO_REPLY)replyBuffer;
			InterlockedExchange(&keepRunning, 0);

			snprintf(buf_suc_status, sizeof(buf_suc_status), "status: %ld; ping: %ld ms", echoReply->Status, echoReply->RoundTripTime);
			snprintf(buf_suc_time, sizeof(buf_suc_time), "succeeded after %u min %u sec", time_sec_local / 60, time_sec_local % 60);
			printfColor(FCOLOR_GREEN, BCOLOR_NULL, "%s\n", buf_suc_status);
			printfColor(FCOLOR_GREEN, BCOLOR_NULL, "%s\n", buf_suc_time);

			PlaySound("ping_timeout.wav", NULL, SND_FILENAME);
			snprintf(buf_message_box, sizeof(buf_message_box), "%s\n%s", buf_suc_status, buf_suc_time);
#ifdef SHOW_MESSAGE
			MessageBox(NULL, buf_message_box, "[*] ping timeout success", MB_ICONINFORMATION);
#else
			getchar();
#endif
			break;
		} else {
			printfColor(FCOLOR_GRAY, BCOLOR_NULL, "failed with error: %ld; time spent: %u min %u sec\n",
				GetLastError(), time_sec_local / 60, time_sec_local % 60);
		}
		Sleep(ONE_SECOND_MS);
	}

	free(replyBuffer);

exit_handle:
	IcmpCloseHandle(hIcmpFile);

exit:
	printf("exiting...\n");
	loadConsoleAttributes(consoleAttributes);
	return ret;
}
