/*
   Copyright (c) 2015, Digi International Inc.

   Permission to use, copy, modify, and/or distribute this software for any
   purpose with or without fee is hereby granted, provided that the above
   copyright notice and this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
   WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
   ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
/****************************************************************************

   WiFiScan.c

   This code demostrates how to scan WiFi channels for SSID's using the
   ifconfig IFS_WIFI_SCAN option.  The scan takes a while to complete, so it
   calls a callback function when it is done.

   Important features to note:

   -	The ifconfig IFS_WIFI_SCAN option does not return data directly, since
   	the scan takes a fair amount of time.  Instead, callback functions are
      used.  The callback function is passed to ifconfig as the only
      parameter to IFS_WIFI_SCAN.

   -  The records passed to the callback function are ephemeral, since another
      scan may occur immediately after the first has completed.  Thus, the
      records need to be used (or copied) during the callback function.

   -  While waiting for user input, it is important to keep the network
      alive by regularly calling tcp_tick(NULL).  This drives the scan, as
      well as any other network processes running.

	This sample demonstrates a WiFi specific feature.  Additional networking
	samples for WiFi can be found in the Samples\tcpip directory.

****************************************************************************/

/*
 * NETWORK CONFIGURATION
 * Please see the function help (Ctrl-H) on TCPCONFIG for instructions on
 * compile-time network configuration.
 */
#define TCPCONFIG 5

#define DISABLE_TCP
#define DISABLE_DNS

#use "dcrtcp.lib"

#memmap xmem

/****************************************************************************
	print_macaddress

	Routine to print out mac_addr types.

****************************************************************************/
void print_macaddress(far unsigned char *addr)
{
	printf("%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2],
	       addr[3], addr[4], addr[5]);
}

/****************************************************************************
	rxsignal_cmp

	qsort comparison, based on rx signal strength.

   Inputs:
      a, b  -- far pointers to _wifi_wln_scan_bss, a struct populated by
               the WIFI SCAN, including rx_signal (relative receive signal
               strength).

	Return value: > 0 if a > b
					  < 0 if a < b
					  0   if a == b

****************************************************************************/
int rxsignal_cmp(far _wifi_wln_scan_bss *a, far _wifi_wln_scan_bss *b) {
	return b->rx_signal - a->rx_signal;
}

/****************************************************************************
	scan_callback

   Prints out the sorted results of a BSS scan.
   Called when WIFI SCAN is complete.

   The argument is a pointer to the wifi_scan_data structure generated by
   the scan.

   We use _f_qsort to sort the data since the data is `far.'  _f_qsort
   requires a comparison function, and we use the rxsignal_cmp() function
   above.

   Inputs:
      data  -- far pointer to wifi_scan_data structure, which contains a
               count of the number of responses, and an array of
               _wifi_wln_scan_bss structures, with the first `count'
               containing valid data for the responses.

****************************************************************************/
char scandone;
root void scan_callback(far wifi_scan_data* data)
{
	uint8 i, j;
	far _wifi_wln_scan_bss *bss;
   char ssid_str[33];

	bss = data->bss;
	// Sort results by signal strength.  Need to use _f_qsort, since bss is
	// far data.
	_f_qsort(bss, data->count, sizeof(bss[0]), rxsignal_cmp);
	// Print out results
   printf("WiFi Scan Results: %d entries\n", data->count);
	printf("Channel  Signal    MAC                Access Point SSID\n");
	printf("-------------------------------------------------------\n");
   for (i = 0; i < data->count; i++) {
      printf("     %2d      %2d    ",
      		 bss[i].channel, bss[i].rx_signal);
      print_macaddress(bss[i].bss_addr);
		wifi_ssid_to_str (ssid_str, bss[i].ssid, bss[i].ssid_len);
	   printf("  [%s]\n", ssid_str);
   }
   scandone = 1;
}

/****************************************************************************
	main

	Scan for BSS's.  Must call tcp_tick(NULL) to drive scanning process.

****************************************************************************/
void main(void)
{
	int val0, val1, i, level;
   unsigned long int end;

	sock_init();

   scandone = 0;

	// Enable Roaming
	ifconfig (IF_WIFI0, IFS_WIFI_ROAM_ENABLE, 1, IFS_END);

	// Set roam event to loss of 20 beacons
  	ifconfig (IF_WIFI0, IFS_WIFI_ROAM_BEACON_MISS, 20, IFS_END);

   // Bring the interface down before starting a scan
	ifdown(IF_WIFI0);
   while (ifpending(IF_WIFI0) != IF_DOWN)
      tcp_tick(NULL);

   // Set the callback before requesting scan
   printf("Starting scan...\n");
   ifconfig(IF_WIFI0, IFS_WIFI_SCAN, scan_callback, IFS_END);
   while ( ! scandone) {
		tcp_tick(NULL);
   }
   printf("\n\nScan done, exiting....");
   exit(0);
}

