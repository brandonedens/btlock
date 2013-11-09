/** @file
 * Implementation of BT lock, software for locking your computer when Bluetooth
 * device strays too far from the machine.
 *
 *==============================================================================
 * Copyright 2013 by Brandon Edens. All Rights Reserved
 *==============================================================================
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author  Brandon Edens
 * @date    2013-11-09
 * @details
 *
 * This software attempts to lock the computer when the computer is not able to
 * classic Bluetooth (BT) ping the given BT address. The basic operation is upon
 * program startup ensure that the hci0 interface is up and functioning, then
 * repeatedly check to see if the screen is not locked and if it is not then
 * ping the remote Bluetooth device and if it cannot be found then lock the
 * computer. Note that this software delays between attempts to ping.
 *
 * TODO - Add ability to configure what hci device to use for pinging and make
 * the delay adjustable via options to the command aka getopt.
 */

/*******************************************************************************
 * Include Files
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 * Constants
 */

/** The Bluetooth device that will be made up before execution of lock check. */
#define HCI_DEV "hci0"

/** Maximum number of characters in a line of text from shell command. */
#define LINE_NMAX (1035)

/** Number of seconds to wait between attempts to lock. */
#define LOCK_CHECK_DELAY (10)

/*******************************************************************************
 * Local Functions
 */

static bool screen_locked(void);

/******************************************************************************/

/**
 * Return true or false depending upon whether or not screen is locked.
 */
static bool screen_locked(void)
{
    FILE *fp = popen("xscreensaver-command --time", "r");
    if (fp == NULL) {
        fprintf(stderr, "Failure to determine screen lock condition.\n");
        return false;
    }

    char line[LINE_NMAX];
    while (fgets(line, sizeof(line) - 1, fp) != NULL) {
        // Look in the output of the xscreensaver command for the words
        // "screen locked" and if we find them we consider the screen currently
        // locked. Otherwise we indicate that the screen is not locked.
        if (strstr(line, "screen locked") != NULL) {
            pclose(fp);
            return true;
        }
    }
    pclose(fp);
    return false;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Please supply BT address ie 11:22:33:44:55:66\n");
        return 0;
    }

    // Ensure that Bluetooth device is up and functioning.
    int ret = system("sudo hciconfig " HCI_DEV " up");
    if (ret != 0) {
        fprintf(stderr, "Falure to bring up the " HCI_DEV " device.\n");
        return 1;
    }

    // Build the command to execute l2ping checking that Bluetooth device is in
    // proximity.
    assert(strlen(argv[1]) + 1 < LINE_NMAX);
    char lock_cmd[LINE_NMAX];
    sprintf(lock_cmd, "l2ping -c 1 %s &> /dev/null", argv[1]);

    while (1) {
        if (!screen_locked()) {
            int ret = system(lock_cmd);
            if (ret != 0) {
                system("xscreensaver-command --lock > /dev/null");
            }
        }
        sleep(LOCK_CHECK_DELAY);
    }
}

