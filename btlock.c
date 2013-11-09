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
#include <getopt.h>
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
 * Macros
 */
/** Print the given message only when verbosity is greater than 0. */
#define VERBOSE(...) do { \
    if (verbosity > 0) { \
        printf(__VA_ARGS__); \
    } \
} while (0)

/*******************************************************************************
 * Local Variables
 */
/**
 * The amount of onscreen chatter to emit when the program is running, this
 * is naturally no message if no malfunctions occur.
 */
static int verbosity = 0;

/*******************************************************************************
 * Local Functions
 */

static bool screen_locked(void);

/******************************************************************************/

/**
 * Continuously attempt to lock computer if Bluetooth ping returns false.
 */
static void lock_loop(char *bt_addr, char *hci_dev, int delay)
{
    char cmd[LINE_NMAX];
    // Ensure that Bluetooth device is up and functioning.
    sprintf(cmd, "sudo hciconfig %s up", hci_dev);
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Falure to bring up the %s device\n", hci_dev);
        exit(1);
    }

    // Build the command to execute l2ping checking that Bluetooth device is in
    // proximity.
    assert(strlen(bt_addr) + 1 < LINE_NMAX);
    sprintf(cmd, "l2ping -c 1 %s &> /dev/null", bt_addr);

    while (1) {
        if (!screen_locked()) {
            VERBOSE("Pinging remote device: %s\n", bt_addr);
            int ret = system(cmd);
            if (ret != 0) {
                VERBOSE("Now locking screen.\n");
                system("xscreensaver-command --lock > /dev/null");
            }
        }
        sleep(delay);
    }
}

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
            VERBOSE("Screen already locked.\n");
            pclose(fp);
            return true;
        }
    }
    pclose(fp);
    return false;
}

int main(int argc, char *argv[])
{
    char hci_dev[LINE_NMAX] = HCI_DEV;
    int lock_delay = LOCK_CHECK_DELAY;

    while (1) {
        static const struct option long_opts[] = {
            {"verbose",   no_argument,       0, 'v'},
            {"interface", required_argument, 0, 'i'},
            {"sleep",     required_argument, 0, 's'},
            {0,           0,                 0,   0}
        };
        int idx = 0;

        int opt = getopt_long(argc, argv, "i:s:v", long_opts, &idx);
        if (opt == -1) {
            break;
        }

        switch (opt) {
        case 'i':
            strcpy(hci_dev, optarg);
            break;
        case 's':
            lock_delay = atoi(optarg);
            break;
        case 'v':
            ++verbosity;
            break;
        default:
            abort();
        }
    }

    if (verbosity > 0) {
        VERBOSE("Using bluetooth address: %s\n", argv[optind]);
        VERBOSE("HCI device: %s\n", hci_dev);
        VERBOSE("and sleep delay: %d\n", lock_delay);

    }

    if (optind >= argc) {
        fprintf(stderr, "Please supply BT address ie 11:22:33:44:55:66\n");
        return 1;
    }
    if (strlen(argv[optind]) != strlen("11:22:33:44:55:66")) {
        fprintf(stderr, "BT address must be of form: 11:22:33:44:55:66\n");
        return 1;
    }

    lock_loop(argv[optind], hci_dev, lock_delay);
}

