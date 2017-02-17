/*
 * alarm_timeout.c:
 *
 * (c) 2017, neolit123 [at] gmail
 * this program is released in the public domain without warranty of any kind!
 *
 * a small Windows console application that will play an alarm sound after
 * N minutes have passed.
 *
 * the alarm can be aborted by pressing CTRL+C.
*/

#include <stdio.h>
#include <signal.h>
#include <windows.h>
#include <timeout_utils.h>

static WORD consoleAttributes;
static LONG keepRunning;

static void signalHandler(int signal)
{
	(void)signal;
	printfColor(FCOLOR_RED, BCOLOR_NULL, "\nctrl+c triggered!\n");
	InterlockedExchange(&keepRunning, 0);
}

static void playAlarmSound()
{
	PlaySound("alarm_timeout.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
}

int main(void)
{
	int ch, scanf_result;
	char msg_buffer[256];
	const char *msg_press_enter_to_abort = "press enter to stop alarm: ";
	const char *msg_alarm_format = "the system will alarm in %.2f %s(s)";
	const FTYPE thread_tick_ms = 100;
	const FTYPE alarm_timeout_minutes = 0; /* alarm_timeout_seconds / 60; */
	FTYPE remaining_minutes = 0, progress_percent = 0, last_ms_passed = 0, ms_passed = 0, ms_user = 0;

	InterlockedExchange(&keepRunning, 1);

	saveConsoleAttributes(&consoleAttributes);
	printfColor(FCOLOR_YELLOW, BCOLOR_NULL, "\n[*] alarm timeout!\n");

	/* setup a CTRL+C handler */
 	signal(SIGINT, signalHandler);

	/* obtain the timeout from the user */
	while (InterlockedExchangeAdd(&keepRunning, 0) && !ms_user) {
		printfColor(FCOLOR_WHITE, BCOLOR_NULL, "\nhow many minutes until alarm?: ");
		fseek(stdin, 0, SEEK_END); /* skip eveything in stdin */
		scanf_result = scanf("%lf", &ms_user);
		ms_user *= 60 * 1000; /* convert to ms */
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

	/*  show a console message that the alarm will start in N minutes */
	snprintf(msg_buffer, sizeof(msg_buffer),
		msg_alarm_format, ms_user / 1000 / 60, "minute");
	printfColor(FCOLOR_CYAN, BCOLOR_NULL, "%s...\n\n", msg_buffer);

	while (InterlockedExchangeAdd(&keepRunning, 0)) {
		if (ms_passed >= ms_user - alarm_timeout_minutes) {

			/* enough time has passed, show message and initiate alarm */
			printfColor(FCOLOR_RED, BCOLOR_NULL, "\nalarm!\n");
			printfColor(FCOLOR_CYAN, BCOLOR_NULL, "\n%s", msg_press_enter_to_abort);

			playAlarmSound();

			/* allow abort */
			fseek(stdin, 0, SEEK_END);
			ch = getchar();
			if (ch == '\n')
				printfColor(FCOLOR_RED, BCOLOR_NULL, "alarm aborted!\n");
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
	loadConsoleAttributes(consoleAttributes);
	return 0;
}
