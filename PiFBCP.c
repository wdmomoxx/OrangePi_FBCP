/*
Develop By KameraSui ~
2017


Feel Free To Use & Distribution
Inspired By

#tasanakorn/rpi-fbcp
https://github.com/tasanakorn/rpi-fbcp

#linux__frameBuffer__操作2--写入和截屏
http://blog.csdn.net/sno_guo/article/details/8439000
*/

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

//转换图像格式
inline static unsigned short int make16color(unsigned char r, unsigned char g, unsigned char b)
{
    return (
               (((r >> 3) & 31) << 11) |
               (((g >> 2) & 63) << 5)  |
               ((b >> 3) & 31)        );
}

//fb设备信息结构体
typedef struct {
    int fbfd;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize;
    char *fbp;
} FBINFO;

//读取FB设备信息
int tryToFillFBInfoForDevice(FBINFO *fbi, const char * fbDev) {
    // Open the file for reading and writing
    fbi->fbfd = open(fbDev, O_RDWR);
    if (!fbi->fbfd) {
        perror("Error: cannot open framebuffer device");
        return 0;
    }
    printf("%s opened successfully.\n", fbDev);
    // Get fixed screen information
    if (ioctl(fbi->fbfd, FBIOGET_FSCREENINFO, &fbi->finfo)) {
        perror("Error reading fixed information");
        return 0;
    }
    // Get variable screen information
    if (ioctl(fbi->fbfd, FBIOGET_VSCREENINFO, &fbi->vinfo)) {
        perror("Error reading variable information");
        return 0;
    }
    printf("%dx%d, %dbpp\n", fbi->vinfo.xres, fbi->vinfo.yres, fbi->vinfo.bits_per_pixel);
    printf("xoffset:%d, yoffset:%d, line_length: %d\n", fbi->vinfo.xoffset, fbi->vinfo.yoffset, fbi->finfo.line_length );
    // Figure out the size of the screen in bytes
    fbi->screensize = fbi->vinfo.xres * fbi->vinfo.yres * fbi->vinfo.bits_per_pixel / 8;
    printf("screensize = %ld\n", fbi->screensize);
    // Map the device to memory
    fbi->fbp = (char *)mmap(0, fbi->screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbi->fbfd, 0);
    if ((int)fbi->fbp == -1) {
        printf("errno : %d\n", errno);
        perror("Error: failed to map framebuffer device to memory");
        return 0;
    }
    printf("The %s framebuffer device was inited successfully.\n", fbDev);
    return 1 ;
}

//释放设备指针
void releaseFBINFO(FBINFO * fbi) {
    if ((int)fbi->fbp != -1)
        munmap(fbi->fbp, fbi->screensize);
    if ((int)fbi->fbfd != -1)
        close(fbi->fbfd);
}



int main(int argc, char ** argv) {

    char devF1[20], devF2[20];

    if (argc != 3) {
        printf("NO INPUT!!\n\nUsage : \"PiFBCP fb_from fb_to\" \nFor Example: PiFBCP 0 1\n\n");
        return 0;
    }
    //simple format mapping
    sprintf(devF1, "/dev/fb%s", argv[1]);
    sprintf(devF2, "/dev/fb%s", argv[2]);

    FBINFO FB0, FB_TFT;
    //perror("sizeof(unsigned short) = %ld\n", sizeof(unsigned short));
    if (tryToFillFBInfoForDevice(&FB0, devF1) == 1) {
        if (tryToFillFBInfoForDevice(&FB_TFT, devF2) == 1) {

            //只能处理相同分辨率的图像cp
            if (FB0.vinfo.xres == FB_TFT.vinfo.xres && FB0.vinfo.xres == FB_TFT.vinfo.xres) {
                //只能处理32bit/24bit图像格式到16bit的转换
                if ((FB0.vinfo.bits_per_pixel ==24 || FB0.vinfo.bits_per_pixel ==32)&&
                    FB_TFT.vinfo.bits_per_pixel==16) {
                    int i,step;
                    unsigned short c;
                    //像素点占用的byte数目
                    step = FB0.vinfo.bits_per_pixel/8;

                    //while 1 复制核心!!如果有硬件加速API,替换这部分内容即可
                    while (1) {
                        for (i = 0; i < FB0.screensize; i += step) {
                            //转换为16位色
                            c = make16color(FB0.fbp[i], FB0.fbp[i + 1], FB0.fbp[i + 2]);
                            //输出到FB设备
                            FB_TFT.fbp[i / step * 2] = c & 0xff;
                            FB_TFT.fbp[i / step * 2 + 1] = c >> 8;

                        }
                        usleep(1000 * 33);//about 30fps!
                    }
                }else{
                    perror("FB Device ColorFormat unmatch!! FBCP terminal!");
                }
            } else {
                perror("FB Device Resolution unmatch!! FBCP terminal!");
            }
            //clear to ofxx
            //clean framebuffer
            releaseFBINFO(&FB_TFT);
        } else {
            perror("FB_TO Init Error!! FBCP terminal!");
        }
        releaseFBINFO(&FB0);
    } else {
        perror("FB_FROM Init Error!! FBCP terminal!");
    }
    return 0;
}