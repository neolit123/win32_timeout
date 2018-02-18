/*
 * ping_timeout.c:
 *
 * (c) 2017, neolit123 [at] gmail
 * this program is released in the public domain without warranty of any kind!
 *
 * a small Windows console application that will play a sound after succeeding
 * or failing to ICMP echo an IPV4 address.
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

#define IP_ADDRESS              "216.58.212.46"
#define SOUND_FILE_SUCCESS      "ping_timeout_success.wav"
#define SOUND_FILE_ERROR        "ping_timeout_error.wav"
#define ONE_SECOND_MS           1000

static LONG keepRunning;
static LONG timeSec;

enum STATE_SWITCH { ST_SUCCESS, ST_ERROR, ST_NONE };

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
	LONG time_sec_local = 0, last_suc_time = 0;

	char ip[128] = IP_ADDRESS;
	char sendData[32] = "data";
	unsigned long ipAddr = INADDR_NONE;
	unsigned int ipLen;
	enum STATE_SWITCH current_state = ST_NONE;
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
	storeConsoleAttributes();

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
		memset(replyBuffer, 0, replySize);
		retVal = IcmpSendEcho(hIcmpFile, ipAddr, sendData, sizeof(sendData),
			NULL, replyBuffer, replySize, ONE_SECOND_MS);
		echoReply = (PICMP_ECHO_REPLY)replyBuffer;

		time_sec_local = InterlockedExchangeAdd(&timeSec, 0);
		if (!InterlockedExchangeAdd(&keepRunning, 0)) /* ctrl + c was pressed */
			break;

		if (retVal != 0 && echoReply->Status == IP_SUCCESS) {
			if (current_state != ST_SUCCESS) {
				current_state = ST_SUCCESS;
				PlaySound(NULL, 0, 0);
				PlaySound(SOUND_FILE_SUCCESS, NULL, SND_FILENAME | SND_ASYNC);
				last_suc_time = time_sec_local;
			}
			printfColor(FCOLOR_GREEN, BCOLOR_NULL, "success; time: %u min %u sec; ping: %ld ms\n",
				last_suc_time / 60, last_suc_time % 60, echoReply->RoundTripTime);
		} else {
			printfColor(FCOLOR_GRAY, BCOLOR_NULL, "failed; error: %ld; status: %ld; time spent: %u min %u sec\n",
				GetLastError(), echoReply->Status, time_sec_local / 60, time_sec_local % 60);
			if (current_state != ST_ERROR) {
				current_state = ST_ERROR;
				PlaySound(NULL, 0, 0);
				PlaySound(SOUND_FILE_ERROR, NULL, SND_FILENAME | SND_ASYNC);
			}
		}
		Sleep(ONE_SECOND_MS);
	}
	free(replyBuffer);

exit_handle:
	IcmpCloseHandle(hIcmpFile);

exit:
	printf("exiting...\n");
	restoreConsoleAttributes();
	return ret;
}
