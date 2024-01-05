#include <iostream>
#include <string>
#include <curl/curl.h>
#include <json/json.h>
#include <cmath> 

const std::string API_KEY = "API_KEY";
const std::string GEOCODING_API_URL = "http://api.openweathermap.org/geo/1.0/direct";

size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

bool getLatLong(const std::string& city, double& latitude, double& longitude) {
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string apiUrl = GEOCODING_API_URL + "?q=" + city + "&limit=1&appid=" + API_KEY;

        curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);

        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "Failed to perform HTTP request: " << curl_easy_strerror(res) << std::endl;
            curl_easy_cleanup(curl);
            return false;
        }

        Json::CharReaderBuilder reader;
        Json::Value jsonData;
        std::istringstream responseStream(response);

        if (!Json::parseFromStream(reader, responseStream, &jsonData, nullptr)) {
            std::cerr << "Failed to parse JSON response." << std::endl;
            curl_easy_cleanup(curl);
            return false;
        }

        if (!jsonData.isArray() || jsonData.empty()) {
            std::cerr << "Invalid or empty response from the API." << std::endl;
            curl_easy_cleanup(curl);
            return false;
        }

        latitude = jsonData[0]["lat"].asDouble();
        longitude = jsonData[0]["lon"].asDouble();

        curl_easy_cleanup(curl);
        return true;
    }

    return false;
}

int main() {
    std::string city1, city2;

    std::cout << "Enter the first city: ";
    std::getline(std::cin, city1);
    
    double lat1, lon1;
    if (getLatLong(city1, lat1, lon1)) {
        std::cout << "Latitude: " << lat1 << ", Longitude: " << lon1 << std::endl;
    } else {
        std::cerr << "Failed to retrieve coordinates for the first city." << std::endl;
    }

    std::cout << "Enter the second city: ";
    std::getline(std::cin, city2);

    double lat2, lon2;
    if (getLatLong(city2, lat2, lon2)) {
        std::cout << "Latitude: " << lat2 << ", Longitude: " << lon2 << std::endl;
    } else {
        std::cerr << "Failed to retrieve coordinates for the second city." << std::endl;
    }

    double distance = sqrt((lat1 - lat2)*(lat1 - lat2) + (lon1 - lon2)*(lon1 - lon2));
    std::cout << "Distance: " << distance <<std::endl;

    return 0;
}
