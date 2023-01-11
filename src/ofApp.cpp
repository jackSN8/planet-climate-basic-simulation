#include "ofApp.h"
ofSpherePrimitive atmosphere;
ofTexture atmoTex;
ofPixels atmoPixs;
ofSpherePrimitive sphere;
ofLight light;
ofTexture texture;
ofEasyCam camera;
ofPixels pix;

bool paused = true;

const int groundMapWidth = 400;//Texture x width
const int groundMapHeight = 250;//Texture y width

ofColor groundMap[groundMapWidth][groundMapHeight];
float globalHeightMap[groundMapHeight][groundMapWidth];
float globalTempMap[groundMapHeight][groundMapWidth];



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
float avFlux = 10;// -- solar flux, normalized so that planet with earthlike
//atmosphere has an average surface temperature = avFlux in celcius
const float maxGreenTemp = 35;

const float initialIceAmount = 0.02;//Amount of water initially frozen as ice 
//at start of simulation

float seaLevel;//
const float treeLine = 0.84;
const float snowLine = 0.91;

//Stores proponesitary for it to snow in that area
float snowProb[groundMapHeight][groundMapWidth];
//Stores sum of snowProbs
float snowProbSum = 0;
//Stores actually how much snow is in that area, month is assumed to be october/april
float snowQuantity[groundMapHeight][groundMapWidth];


//hydrosphere
float evapRate = (avFlux + 273) / (273 * 100);//rate at which water evaporates into atm
//per year, scaled to 1 cubic metre, so actual rate per year is liquidWaterMass*evapRate

//arbitary precip rate, means 90% of all water vapour in atm doesn't precipitate in a year
//only 10% does, not known how realistic that is rn
const float precipRate = 0.1;

const float waterMass = 5000;// cubic metres of water for a planet 70 metres radius
//This is the total volotile accessible water avaliable to the planet
float solidWaterMass = waterMass*initialIceAmount;//initially initialiceamount*100% of water is ice
float liquidWaterMass = waterMass * (1-initialIceAmount);//rest is in oceans or lakes or rivers
float visLiquidWaterMass = waterMass * (1 - initialIceAmount);//variable that stores
//how much water is visibly displayed, updates only when actual liquid water mass
//is more than 2% different from visLiquidWaterMass to reduce num of visual updates

float vapourWaterMass = 0;//none in atmosphere at start, maybe to be changed
float humidity = 0;
//Map that stores likelhood of precipitation falling in given cell
//Currently, just depth map + a little randomness, but
//later to be calculated using wind and cloud movement etc
float precipitationMap[groundMapHeight][groundMapWidth];
//Sum of all normalazed local rates of precipitation
//Preciption of a single cell/sum is the percentage of total precipitation on planet
// that the cell recieves
float precipitationMapSum = 0;

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
	pix.allocate(groundMapWidth, groundMapHeight, OF_IMAGE_COLOR_ALPHA);
	atmoPixs.allocate(groundMapWidth, groundMapHeight, OF_IMAGE_COLOR_ALPHA);
	atmoPixs.setColor(ofColor(0, 0, 160, 20));

	
	pix.setColor(ofColor(0,0,0));
	
	//300,000 pix array, pos 0 is r presumably, 1 is g, 2 is b etc
	//loop through
	generateHeightMap();

	//Find possible volumes for the sea
	bool waterVolFlag = false;

	//sea scalar is how much the sea level is affected by temperature. 
	//placeholder for actual climate physics.
	float seaScalar = 0.144 * (5 * log10(-avFlux+50) - 0.002 * pow((avFlux-34), 2));
	for (int i = 0; i < seaHeightResolution; i++)
	{		
		float testLevel = float(i) / seaHeightResolution;
		seaVolumes[i] = calculateSeaVolume(testLevel);
		if (seaVolumes[i] > liquidWaterMass && !waterVolFlag)
		{
			waterVolFlag = true;
			seaLevel = testLevel*seaScalar;
		}
	}
	cout << "Sea Level is "<< seaLevel << "\n";
	setupTexture();

	texture.loadData(pix);
	atmoTex.loadData(atmoPixs);
	//ofLoadImage(texture, "earthmap.jpg");
	gui.setup();
	gui.add(testVar.setup("temp", 10, -20, 60));


	sphere.setPosition(ofGetWidth() * 0.5, ofGetHeight() * .5, 0);
	atmosphere.setPosition(ofGetWidth() * 0.5, ofGetHeight() * .5, 0);


	camera.begin();
	camera.roll(180);
	camera.end();

}



//--------------------------------------------------------------
/// <summary>
/// /Sorry terrible commenting on update as was rewritten while very sleepy
/// to be improved vastly
/// </summary>
void ofApp::update()
{
	if (!paused)
	{
		t += 0.003;
		hydrologicalCycle();
		redrawTexture();
		//cout << "Sea level is " << seaLevel << "\n";

		//Gui updates to variables
		if (testVar != avFlux)
		{
			avFlux = testVar;
			generateHeightMap();
		}
	}	
	if (paused)
	{
		if (testVar != avFlux)
		{
			avFlux = testVar;
			generateHeightMap();
			setupTexture();
			texture.loadData(pix);
		}
	}
}

//--------------------------------------------------------------

/// <summary>
/// /Sorry terrible commenting on draw as was rewritten while very sleepy
/// to be improved vastly
/// </summary>
void ofApp::draw()
{
	if (paused)
	{
		ofEnableDepthTest();//Must go first
			
		sphere.setPosition(ofGetWidth() * 0.5, ofGetHeight() * .5, 0);
		atmosphere.setPosition(ofGetWidth() * 0.5, ofGetHeight() * .5, 0);
		
		texture.bind();
		sphere.draw();
		texture.unbind();
		atmoTex.bind();
		atmosphere.draw();
		atmoTex.unbind();
		ofDisableDepthTest();
		gui.draw();
	}
	else
	{
		ofEnableDepthTest();//Must go first
		
		sphere.setPosition(0,0, 0);
		atmosphere.setPosition(0,0, 0);
		camera.begin();
		ofScale(-1, 1, 1);

		texture.bind();
		sphere.draw();
		texture.unbind();
		atmoTex.bind();
		atmosphere.draw();
		atmoTex.unbind();
		camera.end();
	}

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
			//calculates sun flux at that specific spot
			float solarFlux = ofMap(abs(lat), 0, HALF_PI, 20, -20) + avFlux;
			//calculates actual temperature, currently just arbitary
			//function of height and solar flux
			float temp = solarFlux - 10 * pow((mergedDepth + 0.26), 4);
			globalTempMap[i][j] = temp;
			float snowChance = 0;//snow proponsitary for that cell
			//ignores humidity, chance of precipitation takes into acount humidity
			if (temp < 0)
			{
				snowChance = -temp;
			}
			snowProb[i][j] = snowChance;
			snowProbSum += snowChance;
			//Calculate likelehood of precipitation, again tb heavily improved
			//and made much more realistic
			precipitationMap[i][j] = mergedDepth + ofRandom(-0.2, 0.2);
			if (precipitationMap[i][j] > 1)
			{
				precipitationMap[i][j] = 1;
			}
			precipitationMapSum += precipitationMap[i][j];
		}
	}
}

void ofApp::setupTexture()
{
	float iceSum = 0;
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
			float height = globalHeightMap[i][j];
			float temp = globalTempMap[i][j];

			//depricated temp func//float temp = solarFlux + 20 * (globalHeightMap[i][j] - 0.6);
			// 
			//Calculate how much snow should be placed here initiially
			snowQuantity[i][j] = (snowProb[i][j] / snowProbSum) * waterMass * initialIceAmount;
			iceSum += snowQuantity[i][j];
			//first check for snow
			if (snowQuantity[i][j]>0)
			{
				pixColor = ofColor(255, 250, 250,255);//snow
			}
			else if (temp >= 0 && temp < 4)
			{
				pixColor = ofColor(105, 105, 105,255);//rock
			}
			else if (globalHeightMap[i][j]>seaLevel && temp>=1 && temp<maxGreenTemp)
			{
				pixColor = ofColor(34, 139, 34,255);////greenery		
			}
			else if (globalHeightMap[i][j] > seaLevel && temp >= maxGreenTemp)
			{
				pixColor = ofColor(194, 178, 128,255);////desert	
			}			
			else
			{
				pixColor = ofColor(0, 84, 184);//water
			}

			
			
			//uncomment for hightmap
			//pixColor = ofColor(globalHeightMap[i][j]*255);
			// uncomment for temperature map
			//pixColor = ofMap(temp, -15, 45, 0, 255); //


			//ofColor mergedPix = ofColor(globalHeightMap[i][j]);
			groundMap[i][j] = pixColor;
			
			//map groundMap onto the pixels texture, groundmap is 2d, pixels is 1d
			int k = j + i * groundMapWidth;
			k*= 4;
			pix.setColor(k, groundMap[i][j].r);
			pix.setColor(k+1, groundMap[i][j].g);
			pix.setColor(k+2, groundMap[i][j].b);
			pix.setColor(k + 3, groundMap[i][j].a);
		}
	}

	cout << "Mass of all ice at beginning is " << iceSum << "\n";
	cout << "Mass of all water is " << waterMass << "\n";
}

void ofApp::redrawTexture()
{
	float iceSum = 0;
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
			float solarFlux = ofMap(abs(lat), 0, HALF_PI, 20, -20) + avFlux;
			//calculates actual temperature, currently just arbitary
			//function of height and solar flux
			float height = globalHeightMap[i][j];
			float temp = globalTempMap[i][j];

			//depricated temp func//float temp = solarFlux + 20 * (globalHeightMap[i][j] - 0.6);
			// 
			//Calculate how much snow should be placed here initiially
			//snowQuantity[i][j] = (snowProb[i][j] / snowProbSum) * waterMass * initialIceAmount;
			iceSum += snowQuantity[i][j];
			//first check for snow
			if (snowQuantity[i][j] > 0.0001)
			{

				pixColor = ofColor(255, 250, 250,tanh(snowQuantity[i][j]*20)*255);//snow
			}
			else if (temp >= 0 && temp < 4)
			{
				pixColor = ofColor(105, 105, 105,255);//rock
			}
			else if (globalHeightMap[i][j] > seaLevel && temp >= 1 && temp < maxGreenTemp)
			{
				pixColor = ofColor(34, 139, 34,255);////greenery		
			}
			else if (globalHeightMap[i][j] > seaLevel && temp >= maxGreenTemp)
			{
				pixColor = ofColor(194, 178, 128,255);////desert	
			}
			else
			{
				pixColor = ofColor(0, 84, 184);//water
			}



			//uncomment for hightmap
			//pixColor = ofColor(globalHeightMap[i][j]*255);
			// uncomment for temperature map
			//pixColor = ofMap(temp, -15, 45, 0, 255); //


			//ofColor mergedPix = ofColor(globalHeightMap[i][j]);
			groundMap[i][j] = pixColor;

			//map groundMap onto the pixels texture, groundmap is 2d, pixels is 1d
			int k = j + i * groundMapWidth;
			k *= 4;
			pix.setColor(k, groundMap[i][j].r);
			pix.setColor(k + 1, groundMap[i][j].g);
			pix.setColor(k + 2, groundMap[i][j].b);
			pix.setColor(k + 3, groundMap[i][j].a);
		}
	}
	//cout << snowQuantity[0][0] << " is mount of snow at 0,0 \n";
	texture.loadData(pix);
}


void ofApp::hydrologicalCycle()
{
	evaporationCycle();
	precipiationAndMelt();
	if (liquidWaterMass < 0.98 * visLiquidWaterMass)
	{
		updateSeaLevel();		
		visLiquidWaterMass = liquidWaterMass;
	}

}

void ofApp::evaporationCycle()
{
	vapourWaterMass += liquidWaterMass * evapRate;
	liquidWaterMass -= liquidWaterMass * evapRate;
}

void ofApp::precipiationAndMelt()
{
	//as humidity increases, chance of rain must too
	float precipMass = precipRate * vapourWaterMass;
	int snowPlaces = 0;
	float rainMass = 0;
	float snowMass = 0;
	for (int i = 0; i < groundMapHeight; i++)
	{
		for (int j = 0; j < groundMapWidth; j++)
		{
			float cellPrecipMass;
			cellPrecipMass = precipMass * (precipitationMap[i][j] / precipitationMapSum);
			//If it is somewhat likely for it to snow here, but not below zero
			//	
			rainMass += cellPrecipMass;
			if (snowProb[i][j] > 0 & vapourWaterMass>0)
			{
				//cout << "Snow before fall " << snowQuantity[i][j] << "\n";
				snowQuantity[i][j] += cellPrecipMass;
				//cout << "Snow after fall " << snowQuantity[i][j] << "\n";
				snowMass += cellPrecipMass;
				rainMass -= cellPrecipMass;
				snowPlaces++;

				//cout << "It is snowing " << cellPrecipMass << "\n";
			}
			vapourWaterMass -= cellPrecipMass;

			//Some ice has a probality of melting, which is assumed to
			//flow back into oceans
			//if warm, chance is high,
			//if snow piles up too much, chance is high?
			//depricated//float meltProb = tanh(-snowProb[i][j] + 1);
			if (globalTempMap[i][j] > 0)
			{
				float meltedMass = snowQuantity[i][j];
				liquidWaterMass += meltedMass;
				snowQuantity[i][j] = 0;
			}
			
		}
	}
	liquidWaterMass += rainMass;
	//cout << "It has snowed this year in " << snowPlaces << " this many places. \n";
	//currently all rain falls back to oceans,  
	//flow and rivers to be added
	//liquidWaterMass += precipMass;
	//vapourWaterMass -= precipMass;
	
}


void ofApp::updateSeaLevel()
{
	bool waterVolFlag = false;

	float seaScalar = 0.144 * (5 * log10(-avFlux + 50) - 0.002 * pow((avFlux - 34), 2));
	for (int i = 0; i < seaHeightResolution; i++)
	{
		float testLevel = float(i) / seaHeightResolution;
		if (seaVolumes[i] > liquidWaterMass && !waterVolFlag)
		{
			waterVolFlag = true;
			seaLevel = testLevel * seaScalar;
		}
	}
	redrawTexture();
	//cout << "Sea Level is " << seaLevel << "\n";
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
int ofApp::From2DTo1D(ofColor* inp)
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
void ofApp::keyPressed(int key)
{
	
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key)
{
	if (key == 112)
	{
		paused = !paused;
		cout << paused;
	}
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button)
{
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
