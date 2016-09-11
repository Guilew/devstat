/** @file devstat.c
 * @mainpage Devstat
 *
 *  The purpose of this software is to implement a simple tool to show network
 * interface usage: Packets per second (PPS) and throughput.
 *
 * Usage:
 * devstat [-i <interface>] [-r <refresh interval (seconds)>] [-c <clear>]
 * devstat -i eth0 -r 1
 *  where:
 *  <interface>: Interface to monitor
 *  <refresh interval>: Refresh interval (seconds)
 *  <clear>: Clear screen before printing the new values
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define STR_VERSION "1.0"
#define DEV_STATS_PATH "/proc/net/dev"
#define MAXIFNUM 50 /* Maximum interfaces to monitor */

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#define SUCCESS 0
#define TRUE 1
#define FALSE 0

#ifdef PROG_DEBUG
#define PRINT_DEBUG(fmt, ...) printf ("DBG(%s => %s():%d): "fmt"\n", __FILE__, __FUNCTION__, __LINE__ ,##__VA_ARGS__)
#else
#define PRINT_DEBUG(fmt, ...)
#endif

typedef struct devstat_s
{
	char ifname[IFNAMSIZ + 1];
	unsigned long rxBytes;
	unsigned long rxPackets;
	unsigned long txBytes;
	unsigned long txPackets;
} devstat_t;

int readProc(devstat_t *pDevstat);

char ifname[IFNAMSIZ + 1];
char isAll = TRUE;

void printUsage(void)
{
	printf(
	    "Devstat - version %s\n\nUsage: devstat [-i <interface>] [-r <refresh interval(s)>] [-c <clear>]\n",
	    STR_VERSION);
}

int main(int argc, char *argv[])
{
	int sleepTime = 1;
	int ifNum, c, i; /* getopt */
	float txMbps, rxMbps, txMbpsMax[MAXIFNUM], rxMbpsMax[MAXIFNUM];
	unsigned int txPPS, rxPPS, txPPSMax[MAXIFNUM], rxPPSMax[MAXIFNUM];
	devstat_t devstat[MAXIFNUM], devstatOld[MAXIFNUM], *pDevstat, *pDevstatOld;
	unsigned char clearScreen = FALSE;

	memset(txPPSMax, 0, MAXIFNUM);
	memset(rxPPSMax, 0, MAXIFNUM);
	memset(txMbpsMax, 0, MAXIFNUM);
	memset(rxMbpsMax, 0, MAXIFNUM);

	/* command line options */
	while ((c = getopt(argc, argv, "hi:r:c")) != EOF)
	{
		switch (c)
		{
			case 'i':
				memset(ifname, 0, IFNAMSIZ + 1);
				strncpy(ifname, optarg, IFNAMSIZ);
				PRINT_DEBUG ("Selected interface: \"%s\"", ifname);
				isAll = FALSE;
			break;

			case 'r':
				sleepTime = strtol(optarg, NULL, 10);
				if (sleepTime < 1)
				{
					sleepTime = 1;
				}
			break;

			case 'c':
				clearScreen = TRUE;
			break;

			case 'h':
			default:
				printUsage();
				return SUCCESS;

		}
	}

	ifNum = readProc(&devstatOld[0]);

	while (1)
	{
		sleep(sleepTime);
		ifNum = readProc(&devstat[0]);

		if (ifNum > MAXIFNUM)
		{
			ifNum = MAXIFNUM;
		}
		pDevstat = &devstat[0];
		pDevstatOld = &devstatOld[0];

		for (i = 0; ifNum > 0; --ifNum, ++pDevstat, ++pDevstatOld, ++i)
		{
			txMbps = (float) ((pDevstat->txBytes - pDevstatOld->txBytes) / sleepTime)
			         * 8 / 1000000;
			rxMbps = (float) ((pDevstat->rxBytes - pDevstatOld->rxBytes) / sleepTime)
			         * 8 / 1000000;
			txPPS = (unsigned int) (pDevstat->txPackets - pDevstatOld->txPackets) / sleepTime;
			rxPPS = (unsigned int) (pDevstat->rxPackets - pDevstatOld->rxPackets) / sleepTime;

			// Max tx/rx throughput
			if (txMbps < 10000)
			{
				if (txMbpsMax[i] < txMbps)
				{
					txMbpsMax[i] = txMbps;
				}
			}
			if (rxMbps < 10000)
			{
				if (rxMbpsMax[i] < rxMbps)
				{
					rxMbpsMax[i] = rxMbps;
				}
			}

			// Max txpps/rxpps
			if (txPPS < 1000000)
			{
				if (txPPSMax[i] < txPPS)
				{
					txPPSMax[i] = txPPS;
				}
			}
			if (rxPPS < 1000000)
			{
				if (rxPPSMax[i] < rxPPS)
				{
					rxPPSMax[i] = rxPPS;
				}
			}

			if (clearScreen)
			{
				system("clear");
			}

			printf("Interface %s:\n", pDevstat->ifname);
			printf("Tx/Rx Rate:\t%.2f Mbps / %.2f Mbps - Total: %.2f Mbps\n",
			       txMbps, rxMbps, txMbps + rxMbps);
			printf("Tx/Rx PPS:\t%u / %u - Total: %u - Diff: %d\n", txPPS, rxPPS,
			       txPPS + rxPPS, rxPPS - txPPS);
			printf("Tx/Rx Max Rate:\t%.2f Mbps / %.2f Mbps - Total: %.2f Mbps\n",
			       txMbpsMax[i], rxMbpsMax[i], txMbpsMax[i] + rxMbpsMax[i]);
			printf("Tx/Rx Max PPS:\t%u / %u - Total: %u - Diff: %d\n",
			       txPPSMax[i], rxPPSMax[i], txPPSMax[i] + rxPPSMax[i],
			       rxPPSMax[i] - txPPSMax[i]);
			printf("+-------------------------------------------------------+\n");

			fflush(NULL);

			memcpy(pDevstatOld, pDevstat, sizeof(devstat_t));
		}
	}

	return SUCCESS;
}

/*!
 * @brief Parse interfaces from /proc/net/dev.
 * @param[in] *pDevstat
 * @return number of interfaces or 0 (error).
 */
int readProc(devstat_t *pDevstat)
{
	char buff[1024];
	char line = 0, found = FALSE;
	FILE *fp = fopen(DEV_STATS_PATH, "r");
	char *fmt, *ptrLine;
	char ifCount = MAXIFNUM;
	int ifNum = 0;

	if (!fp)
	{
		printf("Open %s file error.\n", DEV_STATS_PATH);
		return 0;
	}

	/*
	 * Inter-|   Receive                                                |  Transmit
	    face |bytes    packets errs drop fifo frame compressed multicast|bytes packets errs drop fifo colls carrier compressed
	 lo:    2356      32    0    0    0     0          0         0     2356      32    0    0    0     0       0          0
	 eth0: 1217210    9400    0    0    0     8          0        11  1207648    8019    0    0    0     0       0          0
	 eth1:12039952   21982    6    0    0     6          0         0 47000710   34813    0    0    0   821       0          0
	 */

	fmt = "%lu %lu %*lu %*lu %*lu %*lu %*lu %*lu %lu %lu";

	while (fgets(buff, sizeof(buff) - 1, fp))
	{
		line++;
		PRINT_DEBUG ("Line: \"%s\"", buff);

		if (line <= 2)
			continue;

		if (!isAll && !strstr(buff, ifname))
			continue;

		/* Ignores interfaces */
		if (strstr(buff, "lo:"))
			continue;
		if (strstr(buff, "gre0:"))
			continue;
		if (strstr(buff, "br0:"))
			continue;
		if (strstr(buff, "tunl0:"))
			continue;
		/********************************/

		memset(pDevstat, 0, sizeof(devstat_t));

		if (!isAll)
		{
			found = TRUE;
			PRINT_DEBUG ("Found interface %s", ifname);
		}
		ptrLine = strchr(buff, ':');
		*ptrLine = 0;
		/*******************************/
		sscanf(buff, "%s", pDevstat->ifname);
		ptrLine++;

		sscanf(ptrLine, fmt, &pDevstat->rxBytes, &pDevstat->rxPackets,
		       &pDevstat->txBytes, &pDevstat->txPackets);
		PRINT_DEBUG ("Ifname: %s ; Rx Bytes: %lu ; Rx Packets: %lu ; Tx Bytes: %lu ; Tx Packets: %lu",
		             pDevstat->ifname, pDevstat->rxBytes, pDevstat->rxPackets,
		             pDevstat->txBytes, pDevstat->txPackets);

		--ifCount;
		++ifNum;
		++pDevstat;
		if (!ifCount)
		{
			break;
		}

		if (!isAll && found)
		{
			break;
		}
	}

	fclose(fp);
	PRINT_DEBUG ("Interfaces found = %d", ifNum);
	return ifNum;
}

