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

#include <timeout_utils.h>

static WORD consoleAttributes;
static LONG keepRunning;

static void signalHandler(int signal)
{
	(void)signal;
	printfColor(FCOLOR_RED, BCOLOR_NULL, "\nctrl+c triggered!\n");
	InterlockedExchange(&keepRunning, 0);
}

int main(int argc, char **argv)  {

	const unsigned int timeout_ms = 1000;

	HANDLE hIcmpFile;
	DWORD retVal = 0;
	LPVOID replyBuffer = NULL;
	DWORD replySize = 0;
	PICMP_ECHO_REPLY echoReply;

	char ip[128] = "216.58.212.46"; /* google.com */
	char buf_suc_status[128];
	char buf_suc_time[128];
	char buf_message_box[256];
	char sendData[32] = "data";
	unsigned long ipAddr = INADDR_NONE;
	unsigned int ipLen;
	unsigned int attempts = 0;
	unsigned int time_sec;
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
	saveConsoleAttributes(&consoleAttributes);

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
		attempts++;
		time_sec = attempts * (timeout_ms / 1000);

		retVal = IcmpSendEcho(hIcmpFile, ipAddr, sendData, sizeof(sendData),
			NULL, replyBuffer, replySize, timeout_ms);

		if (retVal != 0) {
			echoReply = (PICMP_ECHO_REPLY)replyBuffer;

			snprintf(buf_suc_status, sizeof(buf_suc_status), "status: %ld; ping: %ld ms", echoReply->Status, echoReply->RoundTripTime);
			snprintf(buf_suc_time, sizeof(buf_suc_time), "succeeded after %u min %u sec", time_sec / 60, time_sec % 60);
			printfColor(FCOLOR_GREEN, BCOLOR_NULL, "%s\n", buf_suc_status);
			printfColor(FCOLOR_GREEN, BCOLOR_NULL, "%s\n", buf_suc_time);

			PlaySound("ping_timeout.wav", NULL, SND_FILENAME);
			snprintf(buf_message_box, sizeof(buf_message_box), "%s\n%s", buf_suc_status, buf_suc_time);
			MessageBox(NULL, buf_message_box, "[*] ping timeout success", MB_ICONINFORMATION);

			break;
		} else {
			printfColor(FCOLOR_GRAY, BCOLOR_NULL, "failed with error: %ld; time spent: %u min %u sec\n",
				GetLastError(), time_sec / 60, time_sec % 60);
		}
		Sleep(timeout_ms);
	}

	free(replyBuffer);

exit_handle:
	IcmpCloseHandle(hIcmpFile);

exit:
	printf("exiting...\n");
	loadConsoleAttributes(consoleAttributes);
	return ret;
}
