#ifndef SATYRICON_SOLVER_HPP
#define SATYRICON_SOLVER_HPP

#include "data_structure.hpp"
#include <string>

namespace Satyricon {

class Solution {
public:
    Solution(bool _is_sat, const std::string& _solution) :
        satisaible(_is_sat), solution(_solution) {}

    bool is_sat() { return satisaible; }
    const std::string& get_solution() { return solution; }

private:
    bool satisaible;
    std::string solution;
};

class SATSolver {
public:
    //SATSolver();
    ~SATSolver() {};

    Solution solve(Formula & f);

private:
    void (*preprocessor)(Formula & f) = nullptr;
    void (*PickBranchingVariable)(Formula & f, Assignment & a) = nullptr;
};

} // end namespace Satyricon

#endif
