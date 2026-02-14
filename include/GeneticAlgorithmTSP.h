#pragma once
#include "TSPAlgorithm.h"
#include <algorithm>
#include <cmath>

// Genetic Algorithm for TSP
// Evolutionary optimization using selection, crossover, and mutation
class GeneticAlgorithmTSP : public TSPAlgorithm {
public:
    enum class SelectionMethod {
        Roulette = 0,
        Tournament = 1,
        Rank = 2
    };

    enum class MutationOperator {
        Swap = 0,
        Inversion = 1,
        OXLike = 2
    };

    GeneticAlgorithmTSP(
        int populationSize = 50,
        double mutationRate = 0.02,
        int maxGenerations = 1000,
        double crossoverRate = 0.8,
        SelectionMethod selectionMethod = SelectionMethod::Roulette,
        MutationOperator mutationOperator = MutationOperator::Swap,
        double elitismPercent = 10.0
    ) : _populationSize(populationSize),
        _mutationRate(mutationRate),
        _maxGenerations(maxGenerations),
        _crossoverRate(crossoverRate),
        _selectionMethod(selectionMethod),
        _mutationOperator(mutationOperator),
        _elitismPercent(elitismPercent) {}

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
        if (_generation >= _maxGenerations || _cityCount < 2) {
            _tspState.finished = true;
            return false;
        }

        // Create new population
        std::vector<std::vector<int>> newPopulation;
        newPopulation.reserve(_populationSize);

        // Elitism: keep top-N individuals
        int eliteCount = static_cast<int>(std::round((_elitismPercent / 100.0) * _populationSize));
        if (eliteCount < 1) eliteCount = 1;
        if (eliteCount > _populationSize) eliteCount = _populationSize;

        std::vector<int> sorted = sortIndicesByFitnessDesc();
        for (int i = 0; i < eliteCount; ++i) {
            newPopulation.push_back(_population[sorted[i]]);
        }

        // Generate rest of population through selection, crossover, mutation
        while (static_cast<int>(newPopulation.size()) < _populationSize) {
            int parent1Idx = selectParent();
            int parent2Idx = selectParent();

            std::vector<int> child;
            if (_realDist(_rng) < _crossoverRate) {
                // Order Crossover (OX)
                child = orderCrossover(_population[parent1Idx], _population[parent2Idx]);
            } else {
                child = _population[parent1Idx];
            }

            // Mutation
            if (_realDist(_rng) < _mutationRate) {
                applyMutation(child);
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

    std::vector<int> sortIndicesByFitnessDesc() const {
        std::vector<int> idx(_populationSize);
        for (int i = 0; i < _populationSize; ++i) idx[i] = i;
        std::sort(idx.begin(), idx.end(), [&](int a, int b) {
            return _fitness[a] > _fitness[b];
        });
        return idx;
    }

    int selectParent() {
        switch (_selectionMethod) {
            case SelectionMethod::Tournament:
                return tournamentSelect();
            case SelectionMethod::Rank:
                return rankSelect();
            case SelectionMethod::Roulette:
            default:
                return rouletteSelect();
        }
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

    int rouletteSelect() {
        double totalFitness = 0.0;
        for (double f : _fitness) totalFitness += f;
        if (totalFitness <= 0.0) {
            std::uniform_int_distribution<int> dist(0, _populationSize - 1);
            return dist(_rng);
        }

        double r = _realDist(_rng) * totalFitness;
        double acc = 0.0;
        for (int i = 0; i < _populationSize; ++i) {
            acc += _fitness[i];
            if (acc >= r) return i;
        }
        return _populationSize - 1;
    }

    int rankSelect() {
        std::vector<int> ranked = sortIndicesByFitnessDesc();
        // Linear ranking probabilities: N..1
        int n = static_cast<int>(ranked.size());
        int sumRanks = n * (n + 1) / 2;
        std::uniform_int_distribution<int> pick(1, sumRanks);
        int r = pick(_rng);

        int acc = 0;
        for (int pos = 0; pos < n; ++pos) {
            int rankWeight = n - pos;
            acc += rankWeight;
            if (acc >= r) return ranked[pos];
        }
        return ranked.back();
    }

    // Order Crossover (OX) - preserves relative order of cities
    // If a Start city is set it is kept at position 0
    std::vector<int> orderCrossover(const std::vector<int>& parent1, const std::vector<int>& parent2) {
        int n = static_cast<int>(parent1.size());
        std::vector<int> child(n, -1);

        // Pin Start city at position 0 if present
        int lo = (_startCityIdx >= 0) ? 1 : 0;
        if (_startCityIdx >= 0 && n > 0) {
            child[0] = _startCityIdx;
        }

        if (n - lo < 2) return (n > 0 && _startCityIdx >= 0) ? child : parent1;

        // Select random segment from parent1 (only among non-start positions)
        std::uniform_int_distribution<int> dist(lo, n - 1);
        int start = dist(_rng);
        int end = dist(_rng);
        if (start > end) std::swap(start, end);

        // Copy segment from parent1
        std::set<int> usedCities;
        if (_startCityIdx >= 0) usedCities.insert(_startCityIdx);
        for (int i = start; i <= end; ++i) {
            child[i] = parent1[i];
            usedCities.insert(parent1[i]);
        }

        // Fill remaining positions with cities from parent2 in order
        int childPos = (end + 1) % n;
        if (childPos < lo) childPos = lo; // skip position 0 if pinned
        for (int i = 0; i < n; ++i) {
            int parent2Pos = (end + 1 + i) % n;
            int city = parent2[parent2Pos];
            
            if (usedCities.find(city) == usedCities.end()) {
                // Skip position 0 if it's reserved for Start
                while (childPos < lo || child[childPos] != -1) {
                    childPos = (childPos + 1) % n;
                }
                child[childPos] = city;
                usedCities.insert(city);
                childPos = (childPos + 1) % n;
            }
        }

        return child;
    }

    // Swap mutation - swap two random cities (preserve Start at position 0)
    void swapMutation(std::vector<int>& tour) {
        if (tour.size() < 2) return;
        
        // Don't swap the start city (index 0) if Start is set
        int lo = (_startCityIdx >= 0) ? 1 : 0;
        if (lo >= static_cast<int>(tour.size())) return;
        
        std::uniform_int_distribution<int> dist(lo, static_cast<int>(tour.size()) - 1);
        int i = dist(_rng);
        int j = dist(_rng);
        std::swap(tour[i], tour[j]);
    }

    void inversionMutation(std::vector<int>& tour) {
        if (tour.size() < 3) return;
        int lo = (_startCityIdx >= 0) ? 1 : 0;
        if (lo >= static_cast<int>(tour.size()) - 1) return;

        std::uniform_int_distribution<int> dist(lo, static_cast<int>(tour.size()) - 1);
        int i = dist(_rng);
        int j = dist(_rng);
        if (i > j) std::swap(i, j);
        if (i == j) return;
        std::reverse(tour.begin() + i, tour.begin() + j + 1);
    }

    void applyMutation(std::vector<int>& tour) {
        switch (_mutationOperator) {
            case MutationOperator::Inversion:
                inversionMutation(tour);
                break;
            case MutationOperator::OXLike:
                // keep behavior deterministic and simple: use inversion as stronger operator
                inversionMutation(tour);
                break;
            case MutationOperator::Swap:
            default:
                swapMutation(tour);
                break;
        }
    }

    int _populationSize;
    double _mutationRate;
    int _maxGenerations;
    double _crossoverRate;
    SelectionMethod _selectionMethod;
    MutationOperator _mutationOperator;
    double _elitismPercent;
    int _generation = 0;
    
    std::vector<std::vector<int>> _population;
    std::vector<double> _fitness;
    std::uniform_real_distribution<double> _realDist{0.0, 1.0};
};
