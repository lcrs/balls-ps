// Read from three Kensington trackballs via hidapi
// Keep state as lift/gamma/gain RGB triplets
// Push state to the selected Photoshop Levels adjustment layer
// Reads omg.js in current folder as template for that
// All v. dirt
// lewis@lewissaunders.com

#include <unistd.h>
#include <sys/errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
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
char *templ;

// Build JS from template and send it to Photoshop
void hi(void) {
	char *js;
	char *crypted;
	int plainlen, cryptlen, t, w;
	int ri0, ri1, ro0, ro1, gi0, gi1, go0, go1, bi0, bi1, bo0, bo1;
	
	// Photoshop uses separate knobs when lift or gain go off the ends
	// Its "input" knobs are used when lift is negative, "output" knobs when
	// lift is positive
	// Here ri0 is red input 0, i.e. red input knob at black end of histogram
	// Oh dear
	ri0 = ro0 = 0;
	if(lift.r > 0.0) {
		ro0 = lift.r;
	} else {
		ri0 = -lift.r;
	}
	ri1 = ro1 = 255;
	if(gain.r < 255.0) {
		ro1 = gain.r;
	} else {
		ri1 = 255.0 - (gain.r - 255.0);
	}
	gi0 = go0 = 0;
	if(lift.g > 0.0) {
		go0 = lift.g;
	} else {
		gi0 = -lift.g;
	}
	gi1 = go1 = 255;
	if(gain.g < 255.0) {
		go1 = gain.g;
	} else {
		gi1 = 255.0 - (gain.g - 255.0);
	}
	bi0 = bo0 = 0;
	if(lift.b > 0.0) {
		bo0 = lift.b;
	} else {
		bi0 = -lift.b;
	}
	bi1 = bo1 = 255;
	if(gain.b < 255.0) {
		bo1 = gain.b;
	} else {
		bi1 = 255.0 - (gain.b - 255.0);
	}

	asprintf(&js, templ, ri0, ri1, gamma.r, ro0, ro1, gi0, gi1, gamma.g, go0, go1, bi0, bi1, gamma.b, bo0, bo1);

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

	cryp->EncryptDecrypt(true, crypted, plainlen, crypted, cryptlen, (size_t *) &cryptlen);
	w = write(sock, crypted, cryptlen);

	free(crypted);
	free(js);
}

// Helper for qsort god what decade is this
int cmp(const void *a, const void *b) {
	return strcmp(*((char **)a), *((char **)b));
}

int main(int argc, char **argv) {
	// Read template .js file
	FILE *f;
	int len;
	f = fopen("omg.js", "rb");
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	templ = (char *) malloc(len + 1);
	fread(templ, len, 1, f);

	// Connect to Photoshop
	struct sockaddr_in addr;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(49494);
	connect(sock, (struct sockaddr *) &addr, sizeof(addr));
	fcntl(sock, F_SETFL, O_NONBLOCK);

	// Today's Photoshop password of choice
	cryp = new PSCryptor("popeface");

	// Connect to the USB HID devices
	// Manuf and model IDs of my trackballs hard-coded below
	struct hid_device_info *devlist, *c;
	char* paths[3];
	hid_device *ball1, *ball2, *ball3;
	devlist = hid_enumerate(0x047d, 0x2048);
	c = devlist;
	paths[0] = strdup(c->path);
	c = c->next;
	paths[1] = strdup(c->path);
	c = c->next;
	paths[2] = strdup(c->path);
	qsort(paths, 3, sizeof(char *), cmp);
	ball1 = hid_open_path(paths[2]);
	ball2 = hid_open_path(paths[1]);
	ball3 = hid_open_path(paths[0]);
	hid_set_nonblocking(ball1, 1);
	hid_set_nonblocking(ball2, 1);
	hid_set_nonblocking(ball3, 1);

	// Read from the balls, update state, write to Photoshop
	unsigned char b[256];
	float ring, x, y;
	int r, dirty = 0, pending = 0;
	lift.r = lift.g = lift.b = 0.0;
	gamma.r = gamma.g = gamma.b = 1.0;
	gain.r = gain.g = gain.b = 255.0;
	while(1) {
		r = hid_read(ball1, b, sizeof(b));
		if(r > 0) {
			x = ((float) (signed char) b[1]) / 100.0;
			lift.g += x;
			lift.r -= x / 2.0;
			lift.b -= x / 2.0;
			y = ((float) (signed char) b[2]) / 100.0;
			lift.r += y;
			lift.g -= y / 2.0;
			lift.b -= y / 2.0;
			ring = ((float) (signed char) b[3]) / 1.0;
			lift.r -= ring;
			lift.g -= ring;
			lift.b -= ring;
			dirty = 1;
		}
		r = hid_read(ball2, b, sizeof(b));
		if(r > 0) {
			x = ((float) (signed char) b[1]) / 3000.0;
			gamma.g += x;
			gamma.r -= x / 2.0;
			gamma.b -= x / 2.0;
			y = ((float) (signed char) b[2]) / 3000.0;
			gamma.r += y;
			gamma.g -= y / 2.0;
			gamma.b -= y / 2.0;
			ring = ((float) (signed char) b[3]) / 100.0;
			gamma.r -= ring;
			gamma.g -= ring;
			gamma.b -= ring;
			dirty = 1;
		}
		r = hid_read(ball3, b, sizeof(b));
		if(r > 0) {
			x = ((float) (signed char) b[1]) / 30.0;
			gain.g += x;
			gain.r -= x / 2.0;
			gain.b -= x / 2.0;
			y = ((float) (signed char) b[2]) / 30.0;
			gain.r += y;
			gain.g -= y / 2.0;
			gain.b -= y / 2.0;
			ring = ((float) (signed char) b[3]) / 0.5;
			gain.r -= ring;
			gain.g -= ring;
			gain.b -= ring;
			dirty = 1;
		}
		// Check Photoshop sockets for acks
		r = read(sock, b, sizeof(b));
		if(r > 0) {
			pending--;
		}
		// Try not to flood Photoshop
		if(dirty && (pending < 1)) {
			hi();
			pending++;
			dirty = 0;
		}
		// Bit aggressive but can't be fucked with select() rn
		usleep(500);
	}
}
