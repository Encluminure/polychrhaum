#include "PolychrHAUM.h"

#ifdef BUILD_PC
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
long long millis();
extern int pc_btn_pwr;
extern int pc_btn_1;
extern int pc_btn_2;
#else
#include "Arduino.h"
#endif

namespace polychrhaum {

void PolychrHAUMcommon::setup() {
	btn1.init();
	btn2.init();
	btn_power.init();
	power.init();
	leds.init();
#ifndef BUILD_PC
	pinMode(pin_btn1, INPUT_PULLUP);
	pinMode(pin_btn2, INPUT_PULLUP);
	pinMode(pin_power_btn, INPUT_PULLUP);
	Serial.begin(115200);
#endif
	last_btn_time = millis();
	last_frame_time = millis();
	comm_buffer_idx = 0;
}

void PolychrHAUMcommon::loop_step() {
	long time = millis();

	// Compute button press
	if (time >= last_btn_time + dtms / 4) {
		last_btn_time = time;
#ifdef BUILD_PC
		btn_power.compute(pc_btn_pwr);
		btn1.compute(pc_btn_1);
		btn2.compute(pc_btn_2);
#else
		btn_power.compute(!digitalRead(pin_power_btn));
		btn1.compute(!digitalRead(pin_btn1));
		btn2.compute(!digitalRead(pin_btn2));
#endif
	}

	// Compute animation
	if (time >= last_frame_time + dtms + adj_time) {
		last_frame_time = time;

		// Power management
		if (!power.is_powered()) {
			if (btn_power.touched() || btn1.touched() || btn2.touched())
				power.poweron();
		} else {
			if (btn_power.slpressed())
				power.poweroff();
		}

		// Potentiometers
#ifndef BUILD_PC
		if (pin_pot_light >= 0) {
			int val = analogRead(pin_pot_light) >> 2;
			leds.set_brightness(val);
		}
		if (pin_pot_speed >= 0) {
			int val = analogRead(pin_pot_speed);
			adj_time = (1 - (val / 640.0)) * dtms * 3 - dtms / 2;
			if (val > 640)
				adj_time = 0;
		}
#endif
		// Animation
		leds.update();
		leds.clear();
		if (fct_animate)
			fct_animate();

		// Reset buttons reports
		btn1.endframe();
		btn2.endframe();
		btn_power.endframe();
	}

	// Compute communication
#ifdef BUILD_PC
	pollfd in {STDIN_FILENO, POLLIN, 0};
	while (comm_buffer_idx < sizeof(comm_buffer) && poll(&in, 1, 0) > 0) {
		int r = fread(comm_buffer + comm_buffer_idx, 1, 1, stdin);
		if (r > 0)
			comm_buffer_idx += r;
		else if (r == 0)
			break;
	}
#else
	while (comm_buffer_idx < sizeof(comm_buffer) && Serial.available() > 0) {
		comm_buffer[comm_buffer_idx] = Serial.read();
		comm_buffer_idx++;
	}
#endif
	if (comm_buffer_idx >= sizeof(comm_buffer)) {
		if (comm_buffer[0] == 0x11 && comm_buffer[6] == 0x12) {
			if (fct_communication)
				fct_communication(comm_buffer[1], comm_buffer + 2);
			comm_buffer_idx = 0;
		} else {
			for (unsigned char resync = 1; resync < sizeof(comm_buffer); resync++) {
				if (comm_buffer[resync] == 0x11) {
					memcpy(comm_buffer, comm_buffer + resync, sizeof(comm_buffer) - resync);
					comm_buffer_idx -= resync;
					break;
				}
			}
			if (comm_buffer_idx >= sizeof(comm_buffer))
				comm_buffer_idx = 0;
		}
	}
}

void PolychrHAUMcommon::log(const char *msg) {
#ifdef BUILD_PC
	printf("%s", msg);
	fflush(stdout);
#else
	Serial.print(msg);
#endif
}

void PolychrHAUMcommon::log(int msg) {
#ifdef BUILD_PC
	printf("%d", msg);
	fflush(stdout);
#else
	Serial.print(msg);
#endif
}

void PolychrHAUMcommon::send_data(char type, char data[4]) {
	char msg[8];
	snprintf(msg, sizeof(msg), "\x11%c%c%c%c%c\x12", type, data[0], data[1], data[2], data[3]);
#ifdef BUILD_PC
	fwrite(msg, sizeof(msg) - 1, 1, stdout);
	fflush(stdout);
#else
	Serial.write(msg, sizeof(msg) - 1);
#endif
}

};

