#include <unistd.h>
#include <sys/errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "pscryptor/PSCryptor.h"
#include "hidapi.h"

PSCryptor *cryp;
int sock;
int tid = 0;

struct colour {
	float r, g, b;
} lift, gamma, gain;

double previously = 0.0;

// Send script to Photoshop
void hi(void) {
	const char *templ = "var color = app.foregroundColor; color.rgb.blue = %d; app.foregroundColor = color;";
	char *js;
	char *crypted;
	int plainlen, cryptlen, t, w;

	asprintf(&js, templ, (int) lift.g);

	plainlen = strlen(js) + 3 * sizeof(int);
	cryptlen = PSCryptor::GetEncryptedLength(plainlen);
	crypted = (char *) malloc(cryptlen);

	// Length of message
	t = htonl(cryptlen + sizeof(int));
	write(sock, &t, sizeof(t));

	// Communications status
	t = htonl(0);
	write(sock, &t, sizeof(t));

	// Protocol version
	t = htonl(1);
	memcpy(crypted, &t, sizeof(t));

	// Transaction ID
	t = htonl(tid++);
	memcpy(crypted + sizeof(t), &t, sizeof(t));

	// Content type
	t = htonl(2);
	memcpy(crypted + 2 * sizeof(t), &t, sizeof(t));

	// Our data
	memcpy(crypted + 3 * sizeof(t), js, strlen(js));

	// Do the stuff
	cryp->EncryptDecrypt(true, crypted, plainlen, crypted, cryptlen, (size_t *) &cryptlen);
	w = write(sock, crypted, cryptlen);
	printf("%d\n", w);

	free(crypted);
	free(js);
}

int main(int argc, char **argv) {
	// Connect to Photoshop
	struct sockaddr_in addr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(49494);
	connect(sock, (struct sockaddr *) &addr, sizeof(addr));

	// Setup encryption
	cryp = new PSCryptor("popeface");

	// Connect to the balls
	struct hid_device_info *devlist, *c;
	hid_device *ball1, *ball2, *ball3;
	unsigned char b[256];
	int r, dirty = 0;

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

	// Read from the balls and write to Photoshop
	lift.r = lift.g = lift.b = 0.0;
	gamma.r = gamma.g = gamma.b = 1.0;
	gain.r = gain.g = gain.b = 1.0;
	while(1) {
		r = hid_read(ball1, b, sizeof(b));
		if(r) {
			printf("ball 1 %02hhx %02hhx %02hhx\n", b[1], b[2], b[3]);
			lift.g += ((float) (signed char) b[2]) / 10.0;
			dirty++;
		}
		r = hid_read(ball2, b, sizeof(b));
		if(r) {
			printf("ball 2 %02hhx %02hhx %02hhx\n", b[1], b[2], b[3]);
		}
		r = hid_read(ball3, b, sizeof(b));
		if(r) {
			printf("ball 3 %02hhx %02hhx %02hhx\n", b[1], b[2], b[3]);
		}
		if(dirty) {
			struct timeval tv;
			struct timezone tz;
			double now;
			gettimeofday(&tv, &tz);
			now = tv.tv_usec + (tv.tv_sec * 1000000.0);
			if(now - previously > (1000000.0 / 10.0)) {
				hi();
				previously = now;
				dirty = 0;
			}
		}
		usleep(5000);
	}
}
