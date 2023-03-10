#pragma once

#include "ofMain.h"
#include "ofxGui.h"



class ofApp : public ofBaseApp{

	public:
		//gui variables
		ofxFloatSlider testVar;
		ofxPanel gui;


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
		void redrawTexture();

		//--hydrological functions
		void hydrologicalCycle();
		void evaporationCycle();
		void updateSeaLevel();
		void precipiationAndMelt();
		void iceMelt();
		//--

		float calculateSeaVolume(float inpSeaLevel);
		int From2DTo1D(ofColor *inp);
		float calculateSeaLevel(float temp);
		ofColor mergeColor(ofColor inp1, ofColor inp2);

		

};
