#include "animator.h"
#include "../PolychrHAUM.h"

namespace polychrhaum {

Animator::Animator() : looped(false), run(false), delta(dtms/1000.0), percent(0), out(0) {
}

void Animator::set_duration(float ms) {
	if (ms < 0) return;
	delta = dtms / ms;
}

void Animator::loop(bool active) {
	looped = active;
}

void Animator::updown(bool active) {
	upndown = active;
}

void Animator::start() {
	run = true;
	percent = 0;
}

void Animator::stop() {
	run = false;
}

void Animator::resume() {
	run = true;
}

bool Animator::running() {
	return run;
}

void Animator::init() {
	looped = false;
	run = false;
	delta = dtms / 1000.0;
	percent = 0;
	out = 0;
	init_animator();
}

void Animator::animate() {
	if (run)
		percent += delta;
	if (percent >= 1) {
		percent = 1;
		if (looped)
			start();
		else
			stop();
	}
	float p = percent;
	if (upndown) {
		p *= 2;
		if (p > 1) p = 2 - p;
	}
	out = transfert(p);
}

float Animator::get_value() {
	return out;
}

};

