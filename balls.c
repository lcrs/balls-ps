#include "hidapi.h"

int main(int argc, char **argv) {
	struct hid_device_info *devlist, *c;
	hid_device *ball1, *ball2, *ball3;
	char b[256];

	devlist = hid_enumerate(0x047d, 0x2048);
	c = devlist;
	ball1 = hid_open_path(c->path);
	c = c->next;
	ball2 = hid_open_path(c->path);
	c = c->next;
	ball3 = hid_open_path(c->path);
	hid_free_enumeration(devlist);

}
