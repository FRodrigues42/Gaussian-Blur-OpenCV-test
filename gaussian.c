#include <stdio.h>
#include <stdlib.h>
#include <cv.h>
#include <highgui.h>
#include <math.h>

int blur(IplImage* img, IplImage* imgf, double deviation);
void gaussian(double deviation, int gRadius, double array[]);
double correction(double array[], int q, int gRadius);
inline int gSize(int gRadius)
{
    return gRadius*2+1;
}

int main(int argc, char** argv)
{
    int i, error;
    double deviation;
    IplImage* img;      //original image
    IplImage* imgf;     //blured image
    
    //this program only works for png images so I check the file extension
    for (i=0; argv[1][i]!='\0'; ++i);
    if (argv[1][i-4]!='.' || argv[1][i-3]!='p' || argv[1][i-2]!='n' || argv[1][i-1]!='g')
    {
        printf("File extension not supported!\nPNG only!\n");
        return -1;
    }

    if (!(img=cvLoadImage(argv[1], -1)))
    {
        printf("%s not found!\n",argv[1]);
        return -1;
    }
    imgf = cvCreateImage(cvSize(img->width,img->height),img->depth,img->nChannels);
    //It's necessary to create an image header with the same characteristics of the original image

    deviation=strtod(argv[2],(char **) NULL);
    if (deviation<1/1000000.0)
    {
        printf("Deviation must be positive!\n");
        
        cvReleaseImage(&img);
        cvReleaseImage(&imgf);
        return -1;
    }

    error=blur(img,imgf,deviation);
    if(error<0)
    {
        cvReleaseImage(&img);
        cvReleaseImage(&imgf);
        return -1;
    }

    cvNamedWindow("Gaussian Blur", CV_WINDOW_AUTOSIZE);
    cvShowImage("Gaussian Blur", imgf );
    cvWaitKey(0);       //Stop the program until you press a key
    
    if(!cvSaveImage(argv[3], imgf, 0)) {
        printf("Could not save: %s\n",argv[4]);
    }
    cvReleaseImage(&img);
    cvReleaseImage(&imgf);
    cvDestroyWindow("Gaussian Blur");
    
    return 0;
}

int blur (IplImage* img, IplImage* imgf, double deviation) 
{
    int y,x,i,j;
    int gRadius;
    int safeGuard;
    double c;           //used to save the correction constant
    double a[4];        //used to save the values of BGR (blue, green, red) + transparency
    double* gArray;     //array with gaussian's values
    uchar* ptr;
    uchar* ptrf;        //pointers used to acess the images' pixels
    IplImage *imga;     //auxiliar image

    safeGuard=5;
    if (deviation>0.1) {
        //I empirically formulated this function, +0.5 to round to an int and + a safe value just to be safe :P
        gRadius=(int)(0.5+(deviation*5+2)+safeGuard);
        //The function area pass that point is so small that we can say it's 0.
    }
    else {
        gRadius=7;
    }

    gArray = (double *)malloc((gSize(gRadius))*sizeof(double));
    if (gArray==NULL)   //Always verify if the memory was allocated!
    {
        printf("Memory could not be allocated!");
        return -9001;   //IT'S OVER NINE THOUSAAAAND
    }

    gaussian(deviation,gRadius,gArray);     //saves the gaussian function's areas values to the array
    
    //Horizontal blur
    for (y=0; y<(img->height); ++y)
    {
        //First we "save" a line of the image to simplify
        ptr = (uchar*) (img->imageData + y*(img->widthStep));
        ptrf = (uchar*) (imgf->imageData + y*(imgf->widthStep));

        for (x=0; x<(img->width); ++x)
        {
            a[0]=0;
            a[1]=0;
            a[2]=0;
            a[3]=0;

            if (x<=gRadius) //Initial special case
            {
                //Because we are not doing a convolution with the total area of the gaussian function
                //  we need to increase the value of the convolution
                c=correction(gArray, -x, gRadius);
                //-x value is the relative initial point in the convolution
                
                //Convolution from pixel 0 to the pixel with the gRadius value
                //j is always the relative position to the pixel of the convolution result
                for (j=-x; j<(gRadius+1);++j)   
                {
                    //If we are acessing an outside pixel call an ERROR
                    if ((4*(x+j)+3)>=((img->width)*4+3) || (4*(x+j))<0)
                    {
                        printf("u:%d l:%d w:%d ERROR1a\n",4*(x+j)+3,4*(x+j),img->width*4+3);
                        return -1;
                    }
                    if (j+gRadius<0||j+gRadius>gSize(gRadius)-1)
                    {
                        printf("g:%d ERRORga",j+gRadius);
                        return -3;
                    }
                    
                    //Every pixel in a png image have 4 values
                    //Because j is a relative value we need to add gRadius to find the index of gArray
                    a[0]=a[0]+(ptr[4*(x+j)]*gArray[j+gRadius]*c);       //Blue
                    a[1]=a[1]+(ptr[4*(x+j)+1]*gArray[j+gRadius]*c);     //Green
                    a[2]=a[2]+(ptr[4*(x+j)+2]*gArray[j+gRadius]*c);     //Red
                    a[3]=a[3]+(ptr[4*(x+j)+3]*gArray[j+gRadius]*c);     //Transparency
                }
            }
            else if (x>=((img->width)-gRadius)) //End special case
            {  
                c=correction(gArray,(gRadius+1)-(x-((img->width)-(gRadius+1))),gRadius);
                //(gRadius+1)-(x-((img->width)-(gRadius+1))) value is the relative final+1 point in the convolution
                
                //Convolution from pixel with the final-gRadius value to the final pixel
                for (j=-gRadius; j<(gRadius+1)-(x-((img->width)-(gRadius+1)));++j)
                {
                    if ((4*(x+j)+3)>=((img->width)*4+3) || (4*(x+j))<0)
                    {
                        printf("u:%d l:%d w:%d ERROR1b\n",4*(x+j)+3,4*(x+j),img->width*4+3);
                        return -1;
                    }
                    if (j+gRadius<0||j+gRadius>gSize(gRadius)-1)
                    {
                        printf("g:%d ERRORgb",j+gRadius);
                        return -3;
                    }
                    
                    a[0]=a[0]+(ptr[4*(x+j)]*gArray[j+gRadius]*c);
                    a[1]=a[1]+(ptr[4*(x+j)+1]*gArray[j+gRadius]*c);
                    a[2]=a[2]+(ptr[4*(x+j)+2]*gArray[j+gRadius]*c);
                    a[3]=a[3]+(ptr[4*(x+j)+3]*gArray[j+gRadius]*c);
                }
            }
            else //Normal case
            {
                for (j=-gRadius; j<(gRadius+1);++j)
                {
                    if ((4*(x+j)+3)>=((img->width)*4+3) || (4*(x+j))<0)
                    {
                        printf("u:%d l:%d w:%d ERROR1c\n",4*(x+j)+3,4*(x+j),img->width*4+3);
                        return -1;
                    }
                    if (j+gRadius<0||j+gRadius>gSize(gRadius)-1)
                    {
                        printf("g:%d ERRORgc",j+gRadius);
                        return -3;
                    }
                    
                    //We don't need the correction value because now we use the entire gArray
                    a[0]=a[0]+(ptr[4*(x+j)]*gArray[j+gRadius]);
                    a[1]=a[1]+(ptr[4*(x+j)+1]*gArray[j+gRadius]);
                    a[2]=a[2]+(ptr[4*(x+j)+2]*gArray[j+gRadius]);
                    a[3]=a[3]+(ptr[4*(x+j)+3]*gArray[j+gRadius]);
                    
                }
            }
            
            //Saving the rounded convolution values to the final image
            ptrf[4*x]=(int)(a[0]+0.5);
            ptrf[4*x+1]=(int)(a[1]+0.5);
            ptrf[4*x+2]=(int)(a[2]+0.5);
            ptrf[4*x+3]=(int)(a[3]+0.5);
        }
    }

    imga = cvCreateImage(cvSize(img->width,img->height),img->depth,img->nChannels);
    cvCopy(imgf, imga, NULL);

    //Now we can't simplify because there isn't a "heightStep" in openCV
    //So we need use the pointer of the entire image
    ptr = (uchar*) (imga->imageData);
    ptrf = (uchar*) (imgf->imageData);

    //Vertical blur aplied to the horizontal blur
    for (x=0;x<(img->width);++x)
    {
        for (y=0;y<(img->height);++y)
        {
            a[0]=0;
            a[1]=0;
            a[2]=0;
            a[3]=0;
            
            if (y<=gRadius)
            {
                c=correction(gArray,-y,gRadius);
                
                for (j=-y; j<(gRadius+1);++j)
                {
                    if (y+j>=img->height || y+j<0)
                    {
                        printf("ERROR2a\n");
                        return -2;
                    }
                    if (j+gRadius<0||j+gRadius>gSize(gRadius)-1)
                    {
                        printf("g:%d ERRORgd",j+gRadius);
                        return -3;
                    }
                    //The x is (sort of) fixed and we move up and down in the image by using the widthStep
                    a[0]=a[0]+(ptr[4*x+(y+j)*(img->widthStep)]*gArray[j+gRadius]*c);
                    a[1]=a[1]+(ptr[4*x+1+(y+j)*(img->widthStep)]*gArray[j+gRadius]*c);
                    a[2]=a[2]+(ptr[4*x+2+(y+j)*(img->widthStep)]*gArray[j+gRadius]*c);
                    a[3]=a[3]+(ptr[4*x+3+(y+j)*(img->widthStep)]*gArray[j+gRadius]*c);
                }
            }
            else if (y>=((img->height)-gRadius))
            {
                c=correction(gArray,(gRadius+1)-(y-((img->height)-(gRadius+1))),gRadius);
                
                for (j=-gRadius; j<(gRadius+1)-(y-((img->height)-(gRadius+1)));++j)
                {
                    if (y+j>=img->height || y+j<0)
                    {
                        printf("y:%d j:%d h:%d ERROR2b\n",y,j,img->height);
                        return -2;
                    }
                    if (j+gRadius<0||j+gRadius>gSize(gRadius)-1)
                    {
                        printf("g:%d ERRORge",j+gRadius);
                        return -3;
                    }

                    a[0]=a[0]+(ptr[4*x+(y+j)*(img->widthStep)]*gArray[j+gRadius]*c);
                    a[1]=a[1]+(ptr[4*x+1+(y+j)*(img->widthStep)]*gArray[j+gRadius]*c);
                    a[2]=a[2]+(ptr[4*x+2+(y+j)*(img->widthStep)]*gArray[j+gRadius]*c);
                    a[3]=a[3]+(ptr[4*x+3+(y+j)*(img->widthStep)]*gArray[j+gRadius]*c);
                }
            }
            else
            {
                for (j=-gRadius; j<(gRadius+1);++j)
                {
                    if (y+j>=img->height || y+j<0)
                    {
                        printf("y:%d j:%d h:%d ERROR2c\n",y,j,img->height);
                        return -2;
                    }
                    if (j+gRadius<0||j+gRadius>gSize(gRadius)-1)
                    {
                        printf("g:%d ERRORgf",j+gRadius);
                        return -3;
                    }

                    a[0]=a[0]+(ptr[4*x+(y+j)*(img->widthStep)]*gArray[j+gRadius]);
                    a[1]=a[1]+(ptr[4*x+1+(y+j)*(img->widthStep)]*gArray[j+gRadius]);
                    a[2]=a[2]+(ptr[4*x+2+(y+j)*(img->widthStep)]*gArray[j+gRadius]);
                    a[3]=a[3]+(ptr[4*x+3+(y+j)*(img->widthStep)]*gArray[j+gRadius]);
                }
            }

            ptrf[4*x+y*(img->widthStep)]=(int)(a[0]+0.5);
            ptrf[4*x+1+y*(img->widthStep)]=(int)(a[1]+0.5);
            ptrf[4*x+2+y*(img->widthStep)]=(int)(a[2]+0.5);
            ptrf[4*x+3+y*(img->widthStep)]=(int)(a[3]+0.5);
        }
    }

    cvReleaseImage(&imga); //Always free the memory that you won't need
    free(gArray);

    return 0;
}

void gaussian(double deviation, int gRadius, double array[])
{
    //Now we need to discretize the gaussian function
    //We can't just add the gaussian function's point value to the array because that would lose
    //  an important characteristic: the gaussian function's area is always 1!
    //So we need to take an area with a width of 1 around the wanted point and so the sum of the
    //  the array is 1.
    
    int x,i;
    double y;

    i=0;
    for (x=-gRadius; x<gRadius+1; ++x)
    {
        array[i]=0;
        for (y=x-0.5;y<x+0.5;y=y+1.0/1000000) {
            //Calculates the average point and because the width is 1 is also the area
            array[i]=array[i]+((1/sqrt(M_PI*2*deviation*deviation))*exp(-(y*y)/(2*deviation*deviation)))/1000000.0;
        }
        printf("%d: %f\n",i,array[i]);
        ++i;
    }
}


double correction(double array[], int qq, int gRadius)
{
    //The area of a gaussian function is always 1 but because we not use the entire
    //  array the area is going to be a fraction, so we only need to multiply the 
    //  convolution by a certain constant to make the area of the partial funtion to be 1.
    //The formula I use is: (area not used/area used)+1
    //Example: If the area used is 0.75 then area not used is 0.25 and the constante is 0.25/0.75+1=1.3333
    //          1.3333*0.75=1
    
    int i,border;
    double firstArea,secondArea;

    firstArea=0;
    secondArea=0;

    border=qq+gRadius;  //index position of the array
    
    //Calculating the area
    for (i=0;i<border;++i) {
        firstArea=firstArea+array[i];
    }
    
    //Remember the total area is always 1
    secondArea = (1-firstArea);
    
    //The numerator is always the area that is not used in the covolution
    //  and the area not used is always inferior or equal to 0.5
    if (firstArea<=0.5) {                       //Initial
        return firstArea/secondArea+1;
    }
    else {                                      //Final
        return secondArea/firstArea+1;
    }
}
