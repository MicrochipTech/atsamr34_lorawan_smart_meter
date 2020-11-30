Main.c: test application code

Case 1: No Uplink, S/W timer repeating every 10s WITH SLEEP
•	Log file: test_sans_uplinks_10s_sleep.log
•	It is seen that timer expires properly (approximately) every 10s
•	This shows, there is no issue in s/w timer expiry

Case 2: Uplinks every 10s with s/w timer, WITH sleep, Duty cycle DISABLED
•	Log file: test_with_uplinks_sans_dcycle_10s_sleep_164ms_airtime.log
•	Here DR3 is used with a single byte payload in EU868 band
•	Time-on-air for 1 byte payload in DR3 is 164.864ms (source: Actility’s wireless logger)
•	No extra channels configured
•	RxDelay1 and RxDelay2 are at defaults i.e., 1s and 2s respectively
•	Therefore, it will take (tx_time + 2 + rx_time) at max for a transaction to complete
•	In our case, 2.165+x seconds to complete
•	X can very depending on whether a downlink is received or not
•	If downlink is received, then device slept for 7638ms i.e., 10000ms-(2165ms+downlink_time) (~200ms, 5 byte MAC cmd in DR3)
•	If NO downlink is received, then device slept for 7755ms ie., 10000ms-(2165ms+RX2_window_timeout) (~80ms)
•	ED cannot sleep when MAC is doing transaction i.e., from TX start to RX2_end
•	App timer started immediately after LORAWAN_Send() is called, for 10s
•	MAC transaction starts and it causes PMM to deny sleep until it ends
•	Once the transaction ends, PMM queries the s/w timer remaining timeout and it returns the above mentioned time
•	It then starts sleeping for the duration. So, no problem in sleep or s/w timer.
•	Yet, it is observed, s/w timer expired at expected interval (highlighted)

[Wed May 13 19:11:57.871 2020] testapp initialized and configured
[Wed May 13 19:11:57.874 2020] stack version: MLS_SDK_1_0_P_4
[Wed May 13 19:11:57.909 2020] joining...
[Wed May 13 19:12:03.341 2020] rxdone
[Wed May 13 19:12:03.346 2020] joined
[Wed May 13 19:12:13.341 2020] last sleep time: 9962 ms
[Wed May 13 19:12:13.349 2020] uplink sent status: 8
[Wed May 13 19:12:14.663 2020] rxtmo
[Wed May 13 19:12:15.694 2020] rxdone
[Wed May 13 19:12:15.694 2020] tx_ok
[Wed May 13 19:12:23.343 2020] last sleep time: 7638 ms
[Wed May 13 19:12:23.351 2020] uplink sent status: 8
[Wed May 13 19:12:24.664 2020] rxtmo
[Wed May 13 19:12:25.693 2020] rxdone
[Wed May 13 19:12:25.696 2020] tx_ok
[Wed May 13 19:12:33.348 2020] last sleep time: 7645 ms
[Wed May 13 19:12:33.353 2020] uplink sent status: 8
[Wed May 13 19:12:34.668 2020] rxtmo
[Wed May 13 19:12:35.696 2020] rxdone
[Wed May 13 19:12:35.696 2020] tx_ok
.
.
.
[Wed May 13 19:13:13.366 2020] uplink sent status: 8
[Wed May 13 19:13:14.679 2020] rxtmo
[Wed May 13 19:13:15.606 2020] rxtmo
[Wed May 13 19:13:15.606 2020] tx_ok
[Wed May 13 19:13:23.362 2020] last sleep time: 7755 ms
[Wed May 13 19:13:23.369 2020] uplink sent status: 8
[Wed May 13 19:13:24.681 2020] rxtmo
[Wed May 13 19:13:25.605 2020] rxtmo
[Wed May 13 19:13:25.605 2020] tx_ok
[Wed May 13 19:13:33.360 2020] last sleep time: 7755 ms

Case 3: Uplinks every 10s with s/w timer, WITH sleep, Duty cycle ENABLED
•	Log file: test_with_uplinks_with_dcycle_10s_sleep_164ms_airtime.log
•	Here DR3 is used with a single byte payload in EU868 band
•	Time-on-air for 1 byte payload in DR3 is 164.864ms (source: Actility’s wireless logger)
•	No extra channels configured
•	Duty cycle for default channel is 1%, i.e., for every packet ED waits for ~16.48 seconds
•	RxDelay1 and RxDelay2 are at defaults i.e., 1s and 2s respectively
•	Therefore, it will take (tx_time + 2 + rx_time) at max for a transaction to complete
•	In our case, 2.165+x seconds to complete
•	X can very depending on whether a downlink is received or not
•	If downlink is received, then device slept for 7638ms i.e., 10000ms-(2165ms+downlink_time) (~200ms, 5 byte MAC cmd in DR3)
•	If NO downlink is received, then device slept for 7755ms ie., 10000ms-(2165ms+RX2_window_timeout) (~80ms)
•	Besides, every transaction will cause duty cycle timer to start. It is seen in the log below
•	First transaction is free to start since there is no pending duty cycle. Here the sleep duration is same as the previous case.
•	Second transaction is started but it is denied by MAC (tx_nok_ok) since previous trx started duty cycle for 16.40s
•	As 10s already elapsed, sleep is allowed to run for the remaining 6496ms. Once that expires, and since there is no other task to run, ED sleep again for the remaining duration of app s/w timer (3478ms).
•	Then, the third transaction starts properly and runs as usual.
•	Yet, it is observed, s/w timer expired at expected interval (highlighted)

[Wed May 13 19:03:54.802 2020] testapp initialized and configured
[Wed May 13 19:03:54.806 2020] stack version: MLS_SDK_1_0_P_4
[Wed May 13 19:03:54.837 2020] joining...
[Wed May 13 19:04:00.269 2020] rxdone
[Wed May 13 19:04:00.275 2020] joined
[Wed May 13 19:04:10.270 2020] last sleep time: 9962 ms
[Wed May 13 19:04:10.280 2020] uplink sent status: 8
[Wed May 13 19:04:11.594 2020] rxtmo
[Wed May 13 19:04:12.622 2020] rxdone
[Wed May 13 19:04:12.626 2020] tx_ok
[Wed May 13 19:04:20.274 2020] last sleep time: 7638 ms
[Wed May 13 19:04:20.283 2020] uplink sent status: 8
[Wed May 13 19:04:20.287 2020] tx_not_ok
[Wed May 13 19:04:26.788 2020] last sleep time: 6496 ms
[Wed May 13 19:04:30.275 2020] last sleep time: 3478 ms
[Wed May 13 19:04:30.285 2020] uplink sent status: 8
[Wed May 13 19:04:31.598 2020] rxtmo
[Wed May 13 19:04:32.628 2020] rxdone
[Wed May 13 19:04:32.632 2020] tx_ok
