#include "hidapi.h"

int main(int argc, char **argv) {
	struct hid_device_info *devlist, *c;
	hid_device *ball1, *ball2, *ball3;
	char b[256];
	int r;

	devlist = hid_enumerate(0x047d, 0x2048);
	c = devlist;
	ball1 = hid_open_path(c->path);
	hid_set_nonblocking(ball1, 1);
	c = c->next;
	ball2 = hid_open_path(c->path);
	hid_set_nonblocking(ball2, 1);
	c = c->next;
	ball3 = hid_open_path(c->path);
	hid_set_nonblocking(ball3, 1);
	
	while(1) {
		r = hid_read(ball1, b, sizeof(b));
		if(r) {
			printf("ball 1 %02hhx %02hhx %02hhx\n", b[1], b[2], b[3]);
		}
		r = hid_read(ball2, b, sizeof(b));
		if(r) {
			printf("ball 2 %02hhx %02hhx %02hhx\n", b[1], b[2], b[3]);
		}
		r = hid_read(ball3, b, sizeof(b));
		if(r) {
			printf("ball 3 %02hhx %02hhx %02hhx\n", b[1], b[2], b[3]);
		}
		usleep(5000);
	}

}
