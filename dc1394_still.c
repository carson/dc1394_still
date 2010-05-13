/*
 * 1394-Based Digital Camera Still Test Program
 *
 * still.c:
 * A unit test to save a monochrome still from attached dc1394 cameras.
 * The program will output one pgm for each camera, with the camera's guid
 * as the image's filename.
 *
 * Written by Carson Reynolds <carson@k2.t.u-tokyo.ac.jp>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>                                     
#include <string.h>
#include <inttypes.h>
#include <dc1394/dc1394.h>

#define IMAGE_FILE_EXTENSION ".pgm"
#define MAX_CAMERAS 8
#define NUM_BUFFERS 8

// file to write still
FILE* imagefile;

// iterators
int i, j;

// camera parameters and variables
uint32_t numCameras = 0;
int video_mode;
uint32_t video_mode_width, video_mode_height;
char *frame_buffer=NULL;

// dc1394 variables
dc1394camera_t *cameras[MAX_CAMERAS];
dc1394featureset_t features;
dc1394video_frame_t * frames[MAX_CAMERAS];
dc1394_t * d;
dc1394camera_list_t * list;
dc1394error_t err;

void cleanup(void) {
    int i;
    for (i=0; i < numCameras; i++) {
        dc1394_video_set_transmission(cameras[i], DC1394_OFF);
        dc1394_capture_stop(cameras[i]);
    }
    if (frame_buffer != NULL)
        free( frame_buffer );
}

int main(int argc, char *argv[]) 
{
  // assumes your camera can output 640x480 with 8-bit monochrome
  video_mode = DC1394_VIDEO_MODE_640x480_MONO8;

  d = dc1394_new ();
  if (!d)
    return 1;
  err=dc1394_camera_enumerate (d, &list);
  DC1394_ERR_RTN(err,"Failed to enumerate cameras");

  if (list->num == 0) {
    dc1394_log_error("No cameras found");
    return 1;
  }

  // use two counters so tht cameras array does not contain gaps in the case of errors
  j = 0;
  for (i = 0; i < list->num; i++) {
    if (j >= MAX_CAMERAS)
      break;
    cameras[j] = dc1394_camera_new (d, list->ids[i].guid);
    if (!cameras[j]) {
      dc1394_log_warning("Failed to initialize camera with guid %llx", list->ids[i].guid);
      continue;
    }
    j++;
  }
  numCameras = j;
  dc1394_camera_free_list (list);

  if (numCameras == 0) {
    dc1394_log_error("No cameras found");
    exit (1);
  }

  // setup cameras for capture
  for (i = 0; i < numCameras; i++) {

    err=dc1394_video_set_iso_speed(cameras[i], DC1394_ISO_SPEED_800);
    DC1394_ERR_CLN_RTN(err,cleanup(),"Could not set ISO speed");
    
    err=dc1394_video_set_mode(cameras[i], video_mode);
    DC1394_ERR_CLN_RTN(err,cleanup(),"Could not set video mode");
    
    err=dc1394_capture_setup(cameras[i],NUM_BUFFERS, DC1394_CAPTURE_FLAGS_DEFAULT);
    DC1394_ERR_CLN_RTN(err,cleanup(),"Could not setup camera-\nmake sure that the video mode and framerate are\nsupported by your camera");

    err=dc1394_get_image_size_from_video_mode(cameras[i], video_mode, &video_mode_width, &video_mode_height);
    DC1394_ERR_CLN_RTN(err,cleanup(),"Could not query video mode width and height");

    err=dc1394_video_set_one_shot(cameras[i], DC1394_ON);
    DC1394_ERR_CLN_RTN(err,cleanup(),"Could not use one shot mode");
  }

    fflush(stdout);
    if (numCameras < 1) {
        perror("no cameras found :(\n");
        cleanup();
        exit(-1);
    }

    for (i = 0; i < numCameras; i++) {
      if (dc1394_capture_dequeue(cameras[i], DC1394_CAPTURE_POLICY_WAIT, &frames[i])!=DC1394_SUCCESS)
	dc1394_log_error("Failed to capture from camera %d", i);

      // save image as '[GUID].pgm'
      char filename[256];
      sprintf(filename, "%" PRIu64 "%s",list->ids[i].guid,IMAGE_FILE_EXTENSION);
      imagefile=fopen(filename, "w");

      if( imagefile == NULL)
	{
	  dc1394_log_error("Can't create %s", filename);
	}
  
      // adding the pgm file header
      fprintf(imagefile,"P5\n%u %u 255\n", video_mode_width, video_mode_height);

      // writing to the file
      fwrite((const char *)frames[i]->image, 1, \
	     video_mode_width * video_mode_height, imagefile);
      fclose(imagefile);                                    
    }

  // exit cleanly
  cleanup();
  return(0);
}
