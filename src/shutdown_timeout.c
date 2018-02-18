/*
 * shutdown_timeout.c:
 *
 * (c) 2017, neolit123 [at] gmail
 * this program is released in the public domain without warranty of any kind!
 *
 * a small Windows console application that will perform a shutdown in N
 * minutes if the current user has enough privileges.
 *
 * the shutdown can be aborted by pressing CTRL+C, while the main countdown
 * is in progress or by pressing ENTER, while the final countdown of M seconds
 * is in progress.
*/

#include <stdio.h>
#include <signal.h>
#include <timeout_utils.h>

/* set to 0 to disable the popup window */
#define SHOW_POPUP 1

/* means to enable shutdown priviliges */
static BOOL EnableShutdownPrivileges()
{
	HANDLE hToken = NULL;
	LUID luid;
	TOKEN_PRIVILEGES tp;

	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken);
	LookupPrivilegeValue("", SE_SHUTDOWN_NAME, &luid);
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	return AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, 0);
}

static LONG keepRunning;

static void signalHandler(int signal)
{
	(void)signal;
	printfColor(FCOLOR_RED, BCOLOR_NULL, "\nctrl+c triggered!\n");
	InterlockedExchange(&keepRunning, 0);
}

int main(void)
{
	int ch, scanf_result;
	char msg_buffer[256];
	const char *msg_press_enter_to_abort = "press enter to abort: ";
	const char *msg_shutdown_format = "the system will shutdown in %.2f %s(s)";
	const FTYPE thread_tick_ms = 100;
	const FTYPE shutdown_timeout_seconds = 30, shutdown_timeout_ms = shutdown_timeout_seconds * 1000;
	FTYPE remaining_minutes = 0, progress_percent = 0, last_ms_passed = 0, ms_passed = 0, ms_user = 0;

	InterlockedExchange(&keepRunning, 1);

	saveConsoleAttributes();
	setExecutionState();
	printfColor(FCOLOR_YELLOW, BCOLOR_NULL, "\n[*] shutdown timeout!\n");

	/* setup a CTRL+C handler */
 	signal(SIGINT, signalHandler);

	/* adjust process priviliges */
	if (!EnableShutdownPrivileges()) {
		printfColor(FCOLOR_RED, BCOLOR_NULL, "\n[ERROR] not enough priviliges to perform a shutdown!\n%s", msg_press_enter_to_abort);
		getchar();
		return 1;
	}

	/* obtain the timeout from the user */
	while (InterlockedExchangeAdd(&keepRunning, 0) && !ms_user) {
		printfColor(FCOLOR_WHITE, BCOLOR_NULL, "\nhow many minutes until shutdown?: ");
		fseek(stdin, 0, SEEK_END); /* skip eveything in stdin */
		scanf_result = scanf("%lf", &ms_user);
		ms_user *= 1000 * 60;
		Sleep((DWORD)thread_tick_ms);
		if (!InterlockedExchangeAdd(&keepRunning, 0))
			break;
		if (scanf_result <= 0 || ms_user <= 0) {
			printf("\ninvalid input, try again!\n");
			ms_user = 0;
		}
	}

	if (!InterlockedExchangeAdd(&keepRunning, 0))
		goto exit;

	/*  show a console message that the shutdown will start in N minutes */
	snprintf(msg_buffer, sizeof(msg_buffer),
		msg_shutdown_format, ms_user / 1000 / 60, "minute");
	printfColor(FCOLOR_CYAN, BCOLOR_NULL, "%s...\n\n", msg_buffer);

	while (InterlockedExchangeAdd(&keepRunning, 0)) {
		if (ms_passed >= ms_user - shutdown_timeout_ms) {

			/* enough time has passed, show message and initiate shutdown */
			snprintf(msg_buffer, sizeof(msg_buffer),
				msg_shutdown_format, shutdown_timeout_seconds, "second");
			printfColor(FCOLOR_CYAN, BCOLOR_NULL, "\n%s!\n%s", msg_buffer, msg_press_enter_to_abort);

			InitiateSystemShutdownEx(NULL, (SHOW_POPUP ? msg_buffer : NULL),
				shutdown_timeout_seconds, TRUE, FALSE, 0);

			/* allow abort */
			fseek(stdin, 0, SEEK_END);
			ch = getchar();
			if (ch == '\n') {
				AbortSystemShutdown(NULL);
				printfColor(FCOLOR_RED, BCOLOR_NULL, "shutdown aborted!\n");
			}
			break;
		}

		Sleep((DWORD)thread_tick_ms);
		if (!InterlockedExchangeAdd(&keepRunning, 0))
			break;
		ms_passed += thread_tick_ms;
		/* only show a message once 1 minute has passed */
		if (ms_passed - last_ms_passed >= 1000 * 60) {
			progress_percent = (ms_passed / ms_user) * 100;
			remaining_minutes = roundLocal((ms_user - ms_passed) / 1000 / 60);
			printfColor(FCOLOR_WHITE, BCOLOR_NULL, "progress: %6.2f%% | minutes remaining: %.2f\n",
				progress_percent, remaining_minutes);
			last_ms_passed = ms_passed;
		}
	}

exit:
	printf("\nexiting...\n");
	loadConsoleAttributes();
	restoreExecutionState();
	return 0;
}
