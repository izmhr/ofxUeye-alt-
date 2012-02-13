//
//  ofxUeye.cpp
//  ofxUeye
//
//  Elliot Woods
//	http://www.kimchiandchips.com
//
#include "ofxUeye.h"

//---------------------------------------------------------------
//ofxUeyeDevice
//---------------------------------------------------------------
//
#pragma mark ofxUeyeDevice
ofxUeyeDevice::ofxUeyeDevice(const UEYE_CAMERA_INFO& info) {
	this->cameraID = info.dwCameraID;
	this->deviceID = info.dwDeviceID;
	this->sensorID = info.dwSensorID;
	this->isUse = info.dwInUse == (DWORD)1;
	this->serial = string(info.SerNo);
	this->model = string(info.Model);
	this->status = info.dwStatus;
}

string ofxUeyeDevice::toString() const {
	stringstream output;
	output << this->cameraID << "\t";
	output << this->deviceID << "\t";
	output << this->sensorID << "\t";
	output << (this->isUse ? "active" : "inactive") << "\t";
	output << this->serial << "\t";
	output << this->model << "\t";
	output << this->status;
	return output.str();
}
//
//---------------------------------------------------------------


//---------------------------------------------------------------
//ofxUeyeSensor
//---------------------------------------------------------------
//
ofxUeyeSensor::ofxUeyeSensor(const SENSORINFO& info) {
	this->sensor = info.SensorID;
	this->model = string(info.strSensorName);
	
	switch (info.nColorMode) {
	case IS_COLORMODE_BAYER:
		this->color = Bayer;
		break;
	case IS_COLORMODE_MONOCHROME:
		this->color = Monochrome;
		break;
	case IS_COLORMODE_CBYCRY:
		this->color = CBYCRY;
		break;
	}

	this->width = info.nMaxWidth;
	this->height = info.nMaxHeight;
	this->masterGain = info.bMasterGain;
	this->redGain = info.bRGain;
	this->greenGain = info.bGGain;
	this->blueGain = info.bBGain;
	this->globalShutter = info.bGlobShutter;
	this->pixelSize = (float)info.wPixelSize * 1e-8;
}

ofxUeyeSensor::ofxUeyeSensor() {
	this->sensor = NULL;
	this->width = 0;
	this->height = 0;
}

string ofxUeyeSensor::toString() const {
	stringstream output;
	output << "sensor:\t\t\t\t" << this->sensor << endl;
	output << "model:\t\t\t\t" << this->model << endl;
	output << "color mode:\t\t\t" << this->getColorMode() << endl;
	output << "width:\t\t\t\t" << this->width << endl;
	output << "height:\t\t\t\t" << this->height << endl;
	output << "supports master gain:\t\t" << this->masterGain << endl;
	output << "supports red gain:\t\t" << this->redGain << endl;
	output << "supports green gain:\t\t" << this->greenGain << endl;
	output << "supports blue gain:\t\t" << this->blueGain << endl;
	output << "supports global shutter:\t" << this->globalShutter<< endl;
	return output.str();
}

string ofxUeyeSensor::getColorMode() const {
	switch (this->color) {
	case Bayer:
		return "Bayer";
	case Monochrome:
		return "Monochrome";
	case CBYCRY:
		return "CBYCRY";
	default:
		return "Error";
	}
}
//
//---------------------------------------------------------------

//---------------------------------------------------------------
//ofxUeye
//---------------------------------------------------------------
//
#pragma mark ofxUeye

////
//constructor
ofxUeye::ofxUeye() {
	this->cameraID = 0;
	this->useTexture = true;
}

ofxUeye::~ofxUeye() {
	this->close();
}

////
//list devices

vector<ofxUeyeDevice> ofxUeye::listDevices() {
	const vector<ofxUeyeDevice> devices = ofxUeye::getDevices();

	cout << "////" << endl;
	cout << "//ofxUeye::listDevices (" << devices.size() << " found)" << endl;
	cout << "////" << endl;
	cout << "//" << endl;
	for (int i = 0; i < devices.size(); i++)
		cout << devices[i].toString() << endl;
	cout << "//" << endl;
	cout << "////" << endl;
	cout << endl;

	return devices;
}

vector<ofxUeyeDevice> ofxUeye::getDevices() {
	vector<ofxUeyeDevice> devices;
	int cameraCount;
	UEYE_CAMERA_LIST* list = 0;
	try {
		if (is_GetNumberOfCameras(&cameraCount) != IS_SUCCESS)
			throw;

		list = (UEYE_CAMERA_LIST*) malloc(sizeof(DWORD) + cameraCount * sizeof(UEYE_CAMERA_INFO));
		list->dwCount = cameraCount;

		if (is_GetCameraList(list) != IS_SUCCESS)
			throw;

		for (int i=0; i<cameraCount; i++) {
			UEYE_CAMERA_INFO& info(list->uci[i]);
			devices.push_back(ofxUeyeDevice(list->uci[i]));
		}

	} catch (...) {
		ofLogError() << "ofxUeye::getDevies() error whilst enumerating cameras.";
		return devices;
	}

	if (list != 0)
		free(list);
	return devices;
}

////
//open/close device
bool ofxUeye::init(int deviceID) {
	close();
	HIDS hCam = deviceID;

	int nRet = is_InitCamera(&hCam, NULL);

	////
	//Upload firmware if neccesary
	////
	//
	if (nRet == IS_STARTER_FW_UPLOAD_NEEDED) {
		int timeNeeded;
		is_GetDuration(hCam, IS_SE_STARTER_FW_UPLOAD, &timeNeeded);

		ofLogNotice() << "ofxUeye::init : Updating camera firmware, wait " << timeNeeded << " seconds.";
		hCam = (HIDS) (((INT)hCam) | IS_ALLOW_STARTER_FW_UPLOAD); 
		nRet = is_InitCamera(&hCam, NULL);		
	}
	//
	////

	if (nRet == IS_NO_SUCCESS) {
		ofLogError() << "ofxUeye::init(" << deviceID <<") failed to init camera";
		return false;
	}

	cameraID = hCam;
	SENSORINFO sensor;
	is_GetSensorInfo(hCam, &sensor);
	this->sensor = ofxUeyeSensor(sensor);

	is_SetColorMode(hCam, IS_SET_CM_BAYER);	
	allocatePixels(this->sensor.width, this->sensor.height);

	is_AllocImageMem(hCam, this->sensor.width, this->sensor.height, 8, &data, &dataID);
	is_SetImageMem(hCam, data, dataID);
	ofLogNotice() << "ofxUeye::init : Camera " << deviceID << " initialised successfully";

	return true;
}

bool ofxUeye::init(int width, int height, int deviceID) {
	if (!init(deviceID))
		return false;
	return open(width, height) && startFreeRunCapture();
}

bool ofxUeye::init(const ofxUeyeDevice& device) {
	return init(device.cameraID);
}

void ofxUeye::close() {
	if (!this->isOpen())
		return;
	ofLogNotice() << "ofxUeye::close : closing camera " << cameraID;
	is_ExitCamera(cameraID);
	cameraID = 0;
}

bool ofxUeye::isOpen() const {
	return cameraID != 0;
}

bool ofxUeye::startFreeRunCapture() {
	if (!this->isOpen()) {
		ofLogError() << "ofxUeye::startFreeRunCapture : cannot begin freerun capture as no device is intialised";
		return false;
	}

	return is_CaptureVideo(cameraID, IS_DONT_WAIT) == IS_SUCCESS;
}

void ofxUeye::stopFreeRunCapture() {
	if (!this->isOpen()) {
		ofLogError() << "ofxUeye::stopFreeRunCapture : cannot stop freerun as no camera is initialised";
		return;
	}
}
////
//sensor
const ofxUeyeSensor& ofxUeye::getSensor() const {
	return this->sensor;
}

int ofxUeye::getSensorWidth() const {
	return this->getSensor().width;
}

int ofxUeye::getSensorHeight() const {
	return this->getSensor().height;
}

////
//set properties
bool ofxUeye::open(int width, int height) {
	if (!this->isOpen()) {
		ofLogError() << "ofxUeye::open : cannot open device before it is initialised, call ofxUeye::init first please";
		return false;
	}
	return true;
}

////
//capture
void ofxUeye::capture() {
	if (!isOpen()) {
		ofLogError() << "ofxUeye::capture : cannot capture as device is not open";
		return;
	}

	is_FreezeVideo(cameraID, IS_WAIT);
	int stride = this->getWidth();
	pixels.setFromAlignedPixels((unsigned char*)data, this->getWidth(), this->getHeight(), 1, stride);
	texture.loadData(pixels);
}

////
//ofBaseVideoDraws
////
//
void ofxUeye::draw(float x, float y) {
	this->draw(x, y, this->getWidth(), this->getHeight());
}

void ofxUeye::draw(float x, float y, float w, float h) {
	if (!useTexture)
		return;

	if (texture.isAllocated())
		texture.draw(x, y, w, h);
}

float ofxUeye::getWidth() {
	return pixels.getWidth();
}

float ofxUeye::getHeight() {
	return pixels.getHeight();
}

void ofxUeye::update() {
	this->capture();
}

unsigned char* ofxUeye::getPixels() {
	return this->pixels.getPixels();
}

ofPixels& ofxUeye::getPixelsRef() {
	return this->pixels;
}

bool ofxUeye::isFrameNew() {
	ofLogError() << "ofxUeye::isFrameNew not supported";
	return true;
}

ofTexture& ofxUeye::getTextureReference() {
	return this->texture;
}

void ofxUeye::setUseTexture(bool useTexture) {
	this->useTexture = useTexture;

	if (useTexture) {
		if (this->isOpen())
			this->texture.allocate(this->getWidth(), this->getHeight(), GL_LUMINANCE);
	} else {
		this->texture.clear();
	}
}
//
////

////
//protected
////
//
void ofxUeye::allocatePixels(int width, int height) {
	this->pixels.allocate(width, height, OF_PIXELS_MONO);
	if (useTexture)
		this->texture.allocate(width, height, GL_LUMINANCE);
}