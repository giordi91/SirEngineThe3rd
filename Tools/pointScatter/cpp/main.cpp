#include "nlohmann/json.hpp"
#include <array>
#include <atomic>
#include <cxxopts/cxxopts.hpp>
#include <fstream>
#include <mutex>
#include <random>
#include <stdint.h>
#include <thread>
#include <vector>

inline double Distance(double x1, double y1, double x2, double y2,
                       double imageWidth) {
  // this returns the toroidal distance between the points
  // aka the interval [0, width) wraps around
  double dx = std::abs(double(x2) - double(x1));
  double dy = std::abs(double(y2) - double(y1));

  if (dx > double(imageWidth / 2))
    dx = double(imageWidth) - dx;

  if (dy > double(imageWidth / 2))
    dy = double(imageWidth) - dy;

  // returning squared distance cause why not
  return dx * dx + dy * dy;
}

inline cxxopts::Options getCxxOptions() {
  cxxopts::Options options("Point Scatter 1.0", "scatter points in a tile");
  options.add_options()(
      "o,outPath", "output path for the generate file",
      cxxopts::value<std::string>()->default_value("points.json"))(
      "t,tilesCount", "number of tiles you want to scatter",
      cxxopts::value<std::string>()->default_value("8"))(
      "s,sampleCount", "number of samples per tiles",
      cxxopts::value<std::string>()->default_value("1000"))(
      "d,seed",
      "seed used for the random number, then for multiple tiles seed will be "
      "seed+i",
      cxxopts::value<std::string>()->default_value("0"));
  return options;
}

struct ScatterConfig {
  std::string m_outPath;
  uint32_t m_tileCount = 0;
  uint32_t m_sampleCount = 0;
  uint32_t m_seed = 0;
};

void parseConfig(ScatterConfig &config, int argc, char **argv) {
  auto options = getCxxOptions();
  const cxxopts::ParseResult result = options.parse(argc, argv);

  config.m_outPath = result["outPath"].as<std::string>();
  const std::string tileCountStr = result["tilesCount"].as<std::string>();
  const std::string sampleCountStr = result["sampleCount"].as<std::string>();
  const std::string seedStr = result["seed"].as<std::string>();

  config.m_tileCount = std::stoi(tileCountStr);
  config.m_sampleCount = std::stoi(sampleCountStr);
  config.m_seed = std::stoi(seedStr);
}

int main(int argc, char **argv) {

  ScatterConfig config;
  parseConfig(config, argc, argv);

  std::vector<std::vector<std::array<double, 2>>> finalPoints;

  std::chrono::high_resolution_clock::time_point begin = std::chrono::steady_clock::now();


  std::mutex m;
#pragma omp parallel for
  for (int t = 0; t < config.m_tileCount; ++t) {

    // init random number generator
    std::seed_seq seed{config.m_seed + t};
    std::mt19937 rng(seed);
    double tileSize = 1000.0;
    std::uniform_real_distribution<double> dist(0, tileSize - 1);

    const double c_blueNoiseSampleMultiplier = 1.0;

    std::vector<std::array<double, 2>> samplesPos;
    samplesPos.reserve(config.m_sampleCount);

    uint32_t counter = 0;
    uint32_t printThreashold=0;

    for (uint32_t i = 1; i <= config.m_sampleCount; ++i) {

      uint32_t currentThreshold =
          static_cast<uint32_t>((float(counter) / config.m_sampleCount)*100.0f) /5;
      if (currentThreshold > printThreashold) {
        std::cout << "tile " << t << " progress " << currentThreshold *5<< "%"
                  << std::endl;
        printThreashold = currentThreshold;
      }

      // keep the candidate that is farthest from it's closest point
      size_t numCandidates =
          samplesPos.size() * c_blueNoiseSampleMultiplier + 1;
      double bestDistance = 0.0f;
      double bestCandidateX = 0;
      double bestCandidateY = 0;
      for (size_t candidate = 0; candidate < numCandidates; ++candidate) {
        size_t x = dist(rng);
        size_t y = dist(rng);

        // calculate the closest distance from this point to an existing sample
        double minDist = 9999999.0f;
        for (const std::array<double, 2> &samplePos : samplesPos) {
          double dist = Distance(x, y, samplePos[0], samplePos[1], tileSize);
          if (dist < minDist)
            minDist = dist;
        }

        if (minDist > bestDistance) {
          bestDistance = minDist;
          bestCandidateX = x;
          bestCandidateY = y;
        }
      }
      // normalizing point and pushing back
      samplesPos.push_back({bestCandidateX, bestCandidateY});
      counter += 1;
    }

    size_t sampleCount = samplesPos.size();
    for (int p = 0; p < sampleCount; ++p) {
      samplesPos[p][0] /= tileSize;
      samplesPos[p][1] /= tileSize;
    }
    m.lock();
    finalPoints.push_back(samplesPos);
    m.unlock();
  }

  nlohmann::json j;
  j["tiles"] = finalPoints;

  std::ofstream out(config.m_outPath);
  out << j.dump(4);
  out.close();
std::chrono::high_resolution_clock::time_point end = std::chrono::steady_clock::now();
std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[s]" << std::endl;
}
