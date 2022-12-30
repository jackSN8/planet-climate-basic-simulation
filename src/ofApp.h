#pragma once

#include "ofMain.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		
		void setupTexture();
		void generateHeightMap();
		void updateCloudTexture();


		float calculateSeaVolume(float inpSeaLevel);
		int From2DTo1D(ofColor *inp);
		float calculateSeaLevel(float temp);
		ofColor mergeColor(ofColor inp1, ofColor inp2);

		ofSpherePrimitive sphere;
		
		ofLight light;
		ofTexture texture;
		
		ofEasyCam camera;
		ofPixels pix;


};
