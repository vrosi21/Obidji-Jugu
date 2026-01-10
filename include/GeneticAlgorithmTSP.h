#pragma once
#include "TSPAlgorithm.h"
#include <algorithm>

// Genetic Algorithm for TSP
// Evolutionary optimization using selection, crossover, and mutation
class GeneticAlgorithmTSP : public TSPAlgorithm {
public:
    GeneticAlgorithmTSP(
        int populationSize = 50,
        double mutationRate = 0.02,
        int maxGenerations = 1000
    ) : _populationSize(populationSize),
        _mutationRate(mutationRate),
        _maxGenerations(maxGenerations) {}

    ~GeneticAlgorithmTSP() override = default;

    const char* getName() const override {
        return "Genetic Algorithm";
    }

    int getGeneration() const { return _generation; }
    int getPopulationSize() const { return _populationSize; }

protected:
    void initializeTSP() override {
        _generation = 0;
        _population.clear();
        _fitness.clear();

        // Initialize population with random tours
        _population.reserve(_populationSize);
        _fitness.reserve(_populationSize);

        for (int i = 0; i < _populationSize; ++i) {
            _population.push_back(generateRandomTour());
            _fitness.push_back(calculateFitness(_population.back()));
        }

        // Find initial best
        updateBest();
    }

    bool performStep() override {
        if (_generation >= _maxGenerations || _cityCount < 4) {
            _tspState.finished = true;
            return false;
        }

        // Create new population
        std::vector<std::vector<int>> newPopulation;
        newPopulation.reserve(_populationSize);

        // Elitism: keep the best individual
        int bestIdx = findBestIndex();
        newPopulation.push_back(_population[bestIdx]);

        // Generate rest of population through selection, crossover, mutation
        while (static_cast<int>(newPopulation.size()) < _populationSize) {
            // Tournament selection for two parents
            int parent1Idx = tournamentSelect();
            int parent2Idx = tournamentSelect();

            // Order Crossover (OX)
            std::vector<int> child = orderCrossover(_population[parent1Idx], _population[parent2Idx]);

            // Mutation
            if (_realDist(_rng) < _mutationRate) {
                swapMutation(child);
            }

            newPopulation.push_back(child);
        }

        // Replace population
        _population = std::move(newPopulation);

        // Recalculate fitness
        for (int i = 0; i < _populationSize; ++i) {
            _fitness[i] = calculateFitness(_population[i]);
        }

        // Update best solution
        updateBest();

        _generation++;
        return _generation < _maxGenerations;
    }

private:
    // Calculate fitness (inverse of tour length - shorter is better)
    double calculateFitness(const std::vector<int>& tour) {
        double length = calculateTourLength(tour);
        if (!std::isfinite(length) || length <= 0.0) return 0.0;
        return 1.0 / length;
    }

    // Find index of best individual
    int findBestIndex() const {
        int bestIdx = 0;
        double bestFit = _fitness[0];
        for (int i = 1; i < _populationSize; ++i) {
            if (_fitness[i] > bestFit) {
                bestFit = _fitness[i];
                bestIdx = i;
            }
        }
        return bestIdx;
    }

    // Update best tour found
    void updateBest() {
        int bestIdx = findBestIndex();
        double length = calculateTourLength(_population[bestIdx]);
        
        _tspState.tour = _population[bestIdx];
        _tspState.tourLength = length;

        if (length < _tspState.bestLength) {
            _tspState.bestLength = length;
            _tspState.bestTour = _population[bestIdx];
        }
    }

    // Tournament selection (select best of 3 random individuals)
    int tournamentSelect() {
        std::uniform_int_distribution<int> dist(0, _populationSize - 1);
        
        int best = dist(_rng);
        for (int i = 0; i < 2; ++i) {
            int candidate = dist(_rng);
            if (_fitness[candidate] > _fitness[best]) {
                best = candidate;
            }
        }
        return best;
    }

    // Order Crossover (OX) - preserves relative order of cities
    std::vector<int> orderCrossover(const std::vector<int>& parent1, const std::vector<int>& parent2) {
        int n = static_cast<int>(parent1.size());
        std::vector<int> child(n, -1);

        // Select random segment from parent1
        std::uniform_int_distribution<int> dist(0, n - 1);
        int start = dist(_rng);
        int end = dist(_rng);
        if (start > end) std::swap(start, end);

        // Copy segment from parent1
        std::set<int> usedCities;
        for (int i = start; i <= end; ++i) {
            child[i] = parent1[i];
            usedCities.insert(parent1[i]);
        }

        // Fill remaining positions with cities from parent2 in order
        int childPos = (end + 1) % n;
        for (int i = 0; i < n; ++i) {
            int parent2Pos = (end + 1 + i) % n;
            int city = parent2[parent2Pos];
            
            if (usedCities.find(city) == usedCities.end()) {
                child[childPos] = city;
                usedCities.insert(city);
                childPos = (childPos + 1) % n;
            }
        }

        return child;
    }

    // Swap mutation - swap two random cities
    void swapMutation(std::vector<int>& tour) {
        if (tour.size() < 2) return;
        
        std::uniform_int_distribution<int> dist(0, static_cast<int>(tour.size()) - 1);
        int i = dist(_rng);
        int j = dist(_rng);
        std::swap(tour[i], tour[j]);
    }

    int _populationSize;
    double _mutationRate;
    int _maxGenerations;
    int _generation = 0;
    
    std::vector<std::vector<int>> _population;
    std::vector<double> _fitness;
    std::uniform_real_distribution<double> _realDist{0.0, 1.0};
};
