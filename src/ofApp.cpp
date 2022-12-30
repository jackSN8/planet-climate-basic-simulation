#include "ofApp.h"
ofSpherePrimitive atmosphere;
ofTexture atmoTex;
ofPixels atmoPixs;
const int groundMapWidth = 400;//Texture x width
const int groundMapHeight = 250;//Texture y width

ofColor groundMap[groundMapWidth][groundMapHeight];
float globalHeightMap[groundMapHeight][groundMapWidth];

const int planetRadius = 70;	
const int atmosphereHeight = 2;
const float pass1Step = 0.01;//how fast the noise moves on pass1. scales 
//a distance that is normally 2pi or depending on code the length of texture
const float perlinSeed = 0;
const float atmoSeed = perlinSeed + 437.123;

const int seaHeightResolution = 100;//How many possible sea levels there are
//The higher, the less the sea jumps when changing level

float seaVolumes[seaHeightResolution];//Extremely dumb variable
//stores a range of volumes that the sea could be, depending on sea level


float t = 0;//time

//Factors influencing the planet														
const float avFlux = 40;// -- solar flux, normalized so that planet with earthlike
//atmosphere has an average temperature = flux
const float maxGreenTemp = 35;

float seaLevel;//
const float treeLine = 0.84;
const float snowLine = 0.91;

//hydrosphere
const float waterMass = 5000;// cubic metres of water for a planet 70 metres radius


//atmosphere
const float oxContent = 0.2;
const float n2Content = 0.8;
const float co2Content = 0.01;
const float airPressure = 1;


//--------------------------------------------------------------
void ofApp::setup()
{
	ofEnableAlphaBlending();//nyet
	ofEnableDepthTest();
	ofDisableArbTex();
	sphere.setRadius(planetRadius);
	atmosphere.setRadius(planetRadius+atmosphereHeight);

	atmoPixs.allocate(groundMapWidth, groundMapHeight, OF_IMAGE_COLOR_ALPHA);
	atmoPixs.setColor(ofColor(0, 0, 160, 20));

	pix.allocate(groundMapWidth, groundMapHeight, OF_IMAGE_COLOR);
	pix.setColor(ofColor(0,0,0));
	
	//300,000 pix array, pos 0 is r presumably, 1 is g, 2 is b etc
	//loop through
	generateHeightMap();

	//Find possible volumes for the sea
	bool waterVolFlag = false;
	//how much the sea level is affected by temperature. 
	//placeholder for actual climate physics.
	float seaScalar = 1.3 * (1.5 * log(0.2 * avFlux) - 0.015 * pow((0.2 * avFlux), 2));
	cout << seaScalar;
	for (int i = 0; i < seaHeightResolution; i++)
	{		
		float testLevel = float(i) / seaHeightResolution;
		seaVolumes[i] = calculateSeaVolume(testLevel);
		if (seaVolumes[i] > waterMass && !waterVolFlag)
		{
			waterVolFlag = true;
			seaLevel = testLevel*seaScalar;
		}
	}

	setupTexture();

	texture.loadData(pix);
	atmoTex.loadData(atmoPixs);
	//ofLoadImage(texture, "earthmap.jpg");
}



//--------------------------------------------------------------
void ofApp::update()
{
	//updateCloudTexture();
	atmoTex.loadData(atmoPixs);
	t+=0.003;
}

//--------------------------------------------------------------
void ofApp::draw()
{
	camera.begin();
	//sphere.setPosition(ofGetWidth() * 0.5, ofGetHeight() * .5, 0);
	texture.bind();
	sphere.draw();
	texture.unbind();
	atmoTex.bind();
	atmosphere.draw();
	atmoTex.unbind();
}

void ofApp::generateHeightMap()
{
	for (int i = 0; i < groundMapHeight; i++)
	{
		for (int j = 0; j < groundMapWidth; j++)
		{
			//generates longitude and latitude 
			float lon = ofMap(j, 0, groundMapWidth, -PI, PI);
			float lat = ofMap(i, 0, groundMapHeight, -HALF_PI, HALF_PI);


			//generates a initial pass of perlin at low resolution
			//alphamap causes it so that near international date line, pass has low alpha
			float pass1Alpha = 1 - pow((lon / PI), 12);
			float pass1Depth = ofNoise(j * pass1Step + perlinSeed, i * pass1Step + perlinSeed) * pass1Alpha;
			//Now generate a mirrored perlin map
			float symj = j;
			if (symj > groundMapWidth / 2)//once longitude crosses grenwich, start making it go other way
			{
				symj = groundMapWidth - j;
			}
			//Now generate alpha map for mirrored map, alpha is low everywhere except international
			//date line
			float pass2Alpha = pow((lon / PI), 12);
			float pass2Depth = ofNoise(symj * pass1Step, i * pass1Step) * pass2Alpha;
			//Now merge the two perlin maps at that pixel
			float mergedDepth = (pass1Depth + pass2Depth);

			//Now for a more detailed perlin pass 	
			float pass3Depth = ofNoise(j * pass1Step * 5, i * pass1Step * 5) * pass1Alpha;
			float pass4Depth = ofNoise(symj * pass1Step * 5, i * pass1Step * 5) * pass2Alpha;
			float secondPass = pass3Depth + pass4Depth;
			//second pass has 50% strength of first pass
			mergedDepth = (1.5 * mergedDepth + 0.5 * secondPass) / 2;

			//Now for the extremely detailed pass
			float pass5Depth = ofNoise(j * pass1Step * 25, i * pass1Step * 25) * pass1Alpha;
			float pass6Depth = ofNoise(symj * pass1Step * 25, i * pass1Step * 25) * pass2Alpha;
			float thirdPass = pass5Depth + pass6Depth;
			//third pass has 12.5% strength of first pass and second combined
			mergedDepth = (1.875 * mergedDepth + 0.125 * thirdPass) / 2;
			//Now for the forth major pass set
			float pass7Depth = ofNoise(j * pass1Step * 125, i * pass1Step * 125) * pass1Alpha;
			float pass8Depth = ofNoise(symj * pass1Step * 125, i * pass1Step * 215) * pass2Alpha;
			float forthPass = pass7Depth + pass8Depth;
			//forth pass has 5% strength of first pass and second combined
			mergedDepth = (1.95 * mergedDepth + 0.05 * forthPass) / 2;
			//depth map produces value between 0 and one.
			globalHeightMap[i][j] = mergedDepth;
		}
	}
}

void ofApp::setupTexture()
{
	for (int i = 0; i < groundMapHeight; i++)
	{
		for (int j = 0; j < groundMapWidth; j++)
		{
			//cout << globalHeightMap[54][193];
			//generates longitude and latitude 
			float lon = ofMap(j, 0, groundMapWidth, -PI, PI);
			float lat = ofMap(i, 0, groundMapHeight, -HALF_PI, HALF_PI);

			ofColor pixColor;
			//calculates sun flux at that specific spot
			float solarFlux = ofMap(abs(lat), 0, HALF_PI, 20, -20)+avFlux;
			//calculates actual temperature, currently just arbitary
			//function of height and solar flux
			float temp = solarFlux + 20 * (globalHeightMap[i][j] - 0.6);
			//////pixColor = ofMap(temp, -15, 45, 0, 255); //this generates temperature map
			//first check for snow
			if (temp < 0 || globalHeightMap[i][j]>snowLine)
			{
				pixColor = ofColor(255, 250, 250);//snow
			}
			else if (temp >= 0 && temp < 5 || globalHeightMap[i][j]>treeLine)
			{
				pixColor = ofColor(105, 105, 105);//rock
			}
			else if (globalHeightMap[i][j]>seaLevel && temp>=5 && temp<maxGreenTemp)
			{
				pixColor = ofColor(34, 139, 34);////greenery		
			}
			else if (globalHeightMap[i][j] > seaLevel && temp >= maxGreenTemp)
			{
				pixColor = ofColor(194, 178, 128);////desert	
			}
			else
			{
				pixColor = ofColor(0, 84, 184);//water
			}

			//uncomment for hightmap
			//pixColor = ofColor(globalHeightMap[i][j]*255);

			//ofColor mergedPix = ofColor(globalHeightMap[i][j]);
			groundMap[i][j] = pixColor;
			
			//map groundMap onto the pixels texture, groundmap is 2d, pixels is 1d
			int k = j + i * groundMapWidth;
			k*= 3;
			pix.setColor(k, groundMap[i][j].r);
			pix.setColor(k+1, groundMap[i][j].g);
			pix.setColor(k+2, groundMap[i][j].b);
		}
	}
		


}

void ofApp::updateCloudTexture()
{
	for (int i = 0; i < groundMapHeight; i++)
	{
		for (int j = 0; j < groundMapWidth; j++)
		{
			//generates longitude and latitude 
			float lon = ofMap(j, 0, groundMapWidth, -PI, PI);
			float lat = ofMap(i, 0, groundMapHeight, -HALF_PI, HALF_PI);

			//generates a initial pass of perlin at low resolution
			//alphamap causes it so that near international date line, pass has low alpha
			float pass1Alpha = 1 - pow((lon / PI), 12);
			float pass1Depth = ofNoise(j * pass1Step*5 + atmoSeed+t, i * pass1Step*5 + atmoSeed+t) * pass1Alpha;
			//Now generate a mirrored perlin map
			float symj = j;
			if (symj > groundMapWidth / 2)//once longitude crosses grenwich, start making it go other way
			{
				symj = groundMapWidth - j;
			}
			//Now generate alpha map for mirrored map, alpha is low everywhere except international
			//date line
			float pass2Alpha = pow((lon / PI), 12);
			float pass2Depth = ofNoise(symj * pass1Step*5+atmoSeed+t, i * pass1Step*5+atmoSeed+t) * pass2Alpha;
			//Now merge the two perlin maps at that pixel
			float mergedDepth = (pass1Depth + pass2Depth);

			ofColor atmoCol = ofColor(0, 0, 160, 20);
			if (mergedDepth > 0.8)
			{
				atmoCol = ofColor(180, 180, 180, 50);
			}
			int k = j + i * groundMapWidth;
			k *= 4;
			atmoPixs.setColor(k, atmoCol.r);
			atmoPixs.setColor(k + 1, atmoCol.g);
			atmoPixs.setColor(k + 2, atmoCol.b);
			atmoPixs.setColor(k + 3, atmoCol.a);
		}
	}
}

//takes a position within 2d space and maps it to the equiviliant position
//in a 1d line#
//function tbf
int ofApp::From2DTo1D(ofColor *inp)
{
	for (int i = 0; i < groundMapHeight; i++)
	{
		for (int j = 0; j < groundMapWidth; j++)
		{
			int k = j + groundMapHeight * i;
			k *= 3;//offset to acount for RGB
			return k;
		}
	}
}

float ofApp::calculateSeaVolume(float inpSeaLevel)
{
	float seaVolume = 0;
	for (int i = 0; i < groundMapHeight; i++)
	{
		for (int j = 0; j < groundMapWidth; j++)
		{
			float depth = globalHeightMap[i][j];
			if (depth < inpSeaLevel)
			{
				float area = float(4) * PI * pow(planetRadius, 2) * (1 / (float(groundMapHeight * groundMapWidth)));
				seaVolume += area * (inpSeaLevel - depth);
			}
		}
	}
	return seaVolume;
}

ofColor ofApp::mergeColor(ofColor inp1, ofColor inp2)
{
	int mergedR = (inp1.r + inp2.r) / 2;
	int mergedG = (inp1.g + inp2.g) / 2;
	int mergedB = (inp1.b + inp2.b) / 2;
	return ofColor(mergedR, mergedG, mergedB);

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
