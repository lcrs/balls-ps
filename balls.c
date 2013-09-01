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

// Send script to Photoshop
void hi(void) {
	const char *templ = "var idsetd = charIDToTypeID( 'setd' );\
    var desc13 = new ActionDescriptor();\
    var idnull = charIDToTypeID( 'null' );\
        var ref8 = new ActionReference();\
        var idAdjL = charIDToTypeID( 'AdjL' );\
        var idOrdn = charIDToTypeID( 'Ordn' );\
        var idTrgt = charIDToTypeID( 'Trgt' );\
        ref8.putEnumerated( idAdjL, idOrdn, idTrgt );\
    desc13.putReference( idnull, ref8 );\
    var idT = charIDToTypeID( 'T   ' );\
        var desc14 = new ActionDescriptor();\
        var idpresetKind = stringIDToTypeID( 'presetKind' );\
        var idpresetKindType = stringIDToTypeID( 'presetKindType' );\
        var idpresetKindCustom = stringIDToTypeID( 'presetKindCustom' );\
        desc14.putEnumerated( idpresetKind, idpresetKindType, idpresetKindCustom );\
        var idAdjs = charIDToTypeID( 'Adjs' );\
            var list1 = new ActionList();\
                var desc15 = new ActionDescriptor();\
                var idChnl = charIDToTypeID( 'Chnl' );\
                    var ref9 = new ActionReference();\
                    var idChnl = charIDToTypeID( 'Chnl' );\
                    var idChnl = charIDToTypeID( 'Chnl' );\
                    var idCmps = charIDToTypeID( 'Cmps' );\
                    ref9.putEnumerated( idChnl, idChnl, idCmps );\
                desc15.putReference( idChnl, ref9 );\
                var idGmm = charIDToTypeID( 'Gmm ' );\
                desc15.putDouble( idGmm, %f );\
            var idLvlA = charIDToTypeID( 'LvlA' );\
            list1.putObject( idLvlA, desc15 );\
        desc14.putList( idAdjs, list1 );\
    var idLvls = charIDToTypeID( 'Lvls' );\
    desc13.putObject( idT, idLvls, desc14 );\
executeAction( idsetd, desc13, DialogModes.NO );\
";
	char *js;
	char *crypted;
	int plainlen, cryptlen, t, w;

	asprintf(&js, templ, gamma.g);

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
	fcntl(sock, F_SETFL, O_NONBLOCK);

	// Setup encryption
	cryp = new PSCryptor("popeface");

	// Connect to the balls
	struct hid_device_info *devlist, *c;
	hid_device *ball1, *ball2, *ball3;
	unsigned char b[256];
	int r, dirty = 0, pending = 0;

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
			//printf("ball 1 %02hhx %02hhx %02hhx\n", b[1], b[2], b[3]);
		}
		r = hid_read(ball2, b, sizeof(b));
		if(r) {
			//printf("ball 2 %02hhx %02hhx %02hhx\n", b[1], b[2], b[3]);
			gamma.g += ((float) (signed char) b[2]) / 1000.0;
			dirty = 1;
		}
		r = hid_read(ball3, b, sizeof(b));
		if(r) {
			//printf("ball 3 %02hhx %02hhx %02hhx\n", b[1], b[2], b[3]);
		}
		r = read(sock, b, sizeof(b));
		if(r > 0) {
			pending--;
		}
		if(dirty && pending < 2) {
			hi();
			pending++;
			dirty = 0;
		}
		usleep(5000);
	}
}
