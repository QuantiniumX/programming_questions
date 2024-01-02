#include <functional>
#include <iostream>
#include <map>

void lengthConversion();
void areaConversion();
void volumeConversion();
void weightConversion();
void timeConversion();
void tempConversion();

int main(void) {
  std::string unit{};
  std::cout << "Whats you the unit you wanna change(length, temp, area, "
               "volume, weight, time): ";
  std::cin >> unit;

  std::map<std::string, std::function<void()>> conversionMap = {
      {"length", lengthConversion}, {"area", areaConversion},
      {"volume", volumeConversion}, {"weight", weightConversion},
      {"time", timeConversion},     {"temp", tempConversion},
  };

  auto it = conversionMap.find(unit);
  if (it != conversionMap.end()) {
    it->second();
  } else {
    std::cerr << "Invalid unit: " << unit << std::endl;
  }
  return 0;
}

void lengthConversion() {
  std::cout << "Enter the length you wanna convert from: " << std::endl;
  std::cout << "(m, km, cm, mm, micrometer, nanometer, mila, yard, foot, inch, "
               "light_year): ";
  std::string unit_from{};
  std::cin >> unit_from;

  std::cout << "Enter the length you wanna convert to: " << std::endl;
  std::cout << "(m, km, cm, mm, micrometer, nanometer, mila, yard, foot, inch, "
               "light_year): ";
  std::string unit_to{};
  std::cin >> unit_to;

  std::map<std::string, double> conversion_factors = {
      {"m", 1.0},
      {"km", 0.001},
      {"cm", 100.0},
      {"mm", 1000.0},
      {"micrometer", 1000000.0},
      {"nanometer", 1000000000.0},
      {"mila", 0.0006213689},
      {"yard", 1.0936132983},
      {"foot", 3.280839895},
      {"inch", 39.37007874},
      {"light_year", 1.057008707e-16},
  };

  if (conversion_factors.find(unit_from) != conversion_factors.end() &&
      conversion_factors.find(unit_to) != conversion_factors.end()) {
    double factors_from = conversion_factors[unit_from];
    double factors_to = conversion_factors[unit_to];
    double value;

    std::cout << "Enter value to convert: ";
    std::cin >> value;

    double result = value * (factors_to / factors_from);
    std::cout << value << " " << unit_from << " is equal to " << result << " "
              << unit_to << std::endl;
  }
}
void areaConversion() {
  std::cout << "Enter the area you wanna convert from: " << std::endl;
  std::cout << "(sq_m, sq_km, sq_cm, sq_mm, sq_micrometer, hectare, sq_mila, "
               "sq_yard, sq_foot, sq_inch, "
               "acre): ";
  std::string unit_from{};
  std::cin >> unit_from;

  std::cout << "Enter the area you wanna convert to: " << std::endl;
  std::cout << "(sq_m, sq_km, sq_cm, sq_mm, sq_micrometer, hectare, sq_mila, "
               "sq_yard, sq_foot, sq_inch, "
               "acre): ";
  std::string unit_to{};
  std::cin >> unit_to;

  std::map<std::string, double> conversion_factors = {
      {"sq_m", 1.0},
      {"sq_km", 0.000001},
      {"sq_cm", 10000},
      {"sq_mm", 1000000},
      {"sq_micrometer", 1000000000000},
      {"hectare", 0.0001},
      {"sq_mila", 3.861018768E-7},
      {"sq_yard", 1.1959900463},
      {"sq_foot", 10.763910417},
      {"sq_inch", 1550.0031},
      {"acre", 0.0002471054},

  };

  if (conversion_factors.find(unit_from) != conversion_factors.end() &&
      conversion_factors.find(unit_to) != conversion_factors.end()) {
    double factors_from = conversion_factors[unit_from];
    double factors_to = conversion_factors[unit_to];
    double value;

    std::cout << "Enter value to convert: ";
    std::cin >> value;

    double result = value * (factors_to / factors_from);
    std::cout << value << " " << unit_from << " is equal to " << result << " "
              << unit_to << std::endl;
  }
}
void volumeConversion() {
  std::cout << "Enter the area you wanna convert from: " << std::endl;
  std::cout << "(cu_m, cu_km, cu_cm, cu_mm, litre, milliletre, US_gallon, "
               "US_quart, US_pint, US_cup, US_fluid_ounce, US_table_spoon, "
               "US_tea_spoon, IMP_gallon, IMP_quart, IMP_pint,"
               "IMP_fluid_ounce, IMP_table_spoon, IMP_tea_spoon, cu_mila, "
               "cu_yard, cu_foot, cu_inch): ";
  std::string unit_from{};
  std::cin >> unit_from;

  std::cout << "Enter the area you wanna convert to: " << std::endl;
  std::cout << "(cu_m, cu_km, cu_cm, cu_mm, litre, milliletre, US_gallon, "
               "US_quart, US_pint, US_cup, US_fluid_ounce, US_table_spoon, "
               "US_tea_spoon, IMP_gallon, IMP_quart, IMP_pint,"
               "IMP_fluid_ounce, IMP_table_spoon, IMP_tea_spoon, cu_mila, "
               "cu_yard, cu_foot, cu_inch): ";
  std::string unit_to{};
  std::cin >> unit_to;

  std::map<std::string, double> conversion_factors = {
      {"cu_m", 1.0},
      {"cu_km", 1.E-9},
      {"cu_cm", 1000000},
      {"cu_mm", 1000000000},
      {"litre", 1000},
      {"milliletre", 1000000},
      {"US_gallon", 264.17217686},
      {"US_quart", 1056.6887074},
      {"US_pint", 2113.3774149},
      {"US_cup", 4226.7548297},
      {"US_fluid_ounce", 33814.038638},
      {"US_table_spoon", 67628.077276},
      {"US_tea_spoon", 202884.23183},
      {"IMP_gallon", 219.9692483},
      {"IMP_quart", 879.8769932},
      {"IMP_pint", 1759.7539864},
      {"IMP_fluid_ounce", 35195.079728},
      {"IMP_table_spoon", 56312.127565},
      {"IMP_tea_spoon", 168936.38269},
      {"cu_mila", 2.399128636E-10},
      {"cu_yard", 1.3079506193},
      {"cu_foot", 35.314666721},
      {"cu_inch", 61023.744095},
  };

  if (conversion_factors.find(unit_from) != conversion_factors.end() &&
      conversion_factors.find(unit_to) != conversion_factors.end()) {
    double factors_from = conversion_factors[unit_from];
    double factors_to = conversion_factors[unit_to];
    double value;

    std::cout << "Enter value to convert: ";
    std::cin >> value;

    double result = value * (factors_to / factors_from);
    std::cout << value << " " << unit_from << " is equal to " << result << " "
              << unit_to << std::endl;
  }
}
void weightConversion() {
  std::cout << "Enter the weight you wanna convert from: " << std::endl;
}
void tempConversion() {
    std::unordered_map<std::string, std::unordered_map<std::string, std::function<double(double)>>> conversionMap{
        {"c", {{"k", [](double x) { return x + 273.15; }}, {"f", [](double x) { return x * 9.0 / 5.0 + 32; }}}}, 
        {"f", {{"c", [](double x) { return (x - 32) * 5.0 / 9.0; }}, {"k", [](double x) { return (x - 32) * 5.0 / 9.0 + 273.15; }}}}, 
        {"k", {{"c", [](double x) { return x - 273.15; }}, {"f", [](double x) { return x * 9.0 / 5.0 - 459.67; }}}}
    };

    std::cout << "Enter the unit you want to convert from: " << std::endl;
    std::cout << "(c, k, f): ";
    std::string unit_from{};
    std::cin >> unit_from;

    std::cout << "Enter the unit you want to convert to: " << std::endl;
    std::cout << "(c, k, f): ";
    std::string unit_to{};
    std::cin >> unit_to;

    double value, result;
    std::cout << "Enter the value: ";
    std::cin >> value;

    if (conversionMap.find(unit_from) != conversionMap.end() && conversionMap[unit_from].find(unit_to) != conversionMap[unit_from].end()) {
        result = conversionMap[unit_from][unit_to](value);
        std::cout << value << " " << unit_from << " is equal to " << result << " " << unit_to << std::endl;
    } else {
        std::cerr << "Invalid units entered." << std::endl;
    }
}
void timeConversion() {
  std::cout << "Enter the time you want to convert from: ";
}
