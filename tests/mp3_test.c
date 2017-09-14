#include <sndfile.h>

int main() {
    SF_INFO info;
    SNDFILE * f = sf_open("/home/dave/rockproj/mp3/01 - Many Of Horror.mp3", SFM_READ, &info);
    printf("DOGBAG\n");
    if (f == NULL) {
        printf("Didn't open\n");
    } else {
        printf("Opened, bitchin'\n");
        sf_close(f);
    }
}
