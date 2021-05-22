#include <iostream>
#include <iomanip>
#include <string>
#include <memory>
#include <set>
// #include <unordered_map>
#include <map>
#include <queue>
#include <chrono>
#include <functional>

using namespace std;

/********************************************************************
EXCEPTIONS
********************************************************************/

struct DivisionByZeroException : public exception {
    virtual const char* what() const throw() {
        return "division by zero";
    }
};

struct OpenNodeEvalException : public exception {
    virtual const char* what() const throw() {
        return "cannot evaluate expression tree with open node";
    }
};

struct RequiredLiteralNodeException : public exception {
    virtual const char* what() const throw() {
        return "cannot compute required value of a literal";
    }
};

/********************************************************************
EXPRESSION TREES
********************************************************************/

// expression tree interface
struct Expr {
    // evaluation
    virtual bool evaluable() =0;
    virtual double evaluate() =0;

    // number of open nodes
    virtual int size() =0;

    // replace the left most open node
    virtual shared_ptr<Expr> fill_left(shared_ptr<Expr> expr, bool &done) =0;
    
    // for memoization optimization
    virtual double required(double target) =0;
    virtual set<double> numbers() =0;

    // for heuristics in A*
    virtual double evalaluate_missing(double missing) =0;

    // printing
    virtual string to_string() =0;
};

// function to start off replacement
shared_ptr<Expr> clone_and_fill(shared_ptr<Expr> root, shared_ptr<Expr> repl) {
    bool done = false;
    return root->fill_left(repl, done);
}

// open node in expression tree
struct Open : Expr {
    Open() {}

    virtual bool evaluable() {
        return false;
    }

    virtual double evaluate() {
        throw OpenNodeEvalException();
    }

    virtual int size() {
        return 1;
    }

    virtual shared_ptr<Expr> fill_left(shared_ptr<Expr> expr, bool &done) {
        if (done) {
            return make_shared<Open>();
        } else {
            done = true;
            return expr;
        }
    }

    virtual bool fill_left(shared_ptr<Expr> expr) {
        return true;
    }

    virtual double required(double target) {
        return target;
    }

    virtual set<double> numbers() {
        return set<double>();
    }

    virtual string to_string() {
        return ".";
    }
};

// operator in expression tree
struct Add {
    static const char repr = '+';

    static double eval(double lhs, double rhs) {
        return lhs + rhs;
    }

    static double solve_right(double target, double lhs) {
        return target - lhs;
    }

    static double solve_left(double target, double rhs) {
        return target - rhs;
    }
};

struct Sub {
    static const char repr = '-';

    static double eval(double lhs, double rhs) {
        return lhs - rhs;
    }

    static double solve_right(double target, double lhs) {
        return lhs - target;
    }

    static double solve_left(double target, double rhs) {
        return target + rhs;
    }
};

struct Mul {
    static const char repr = '*';

    static double eval(double lhs, double rhs) {
        return lhs * rhs;
    }

    static double solve_right(double target, double lhs) {
        return target / lhs;
    }

    static double solve_left(double target, double rhs) {
        return target / rhs;
    }
};

struct Div {
    static const char repr = '/';

    static double eval(double lhs, double rhs) {
        if (rhs == 0.0) throw DivisionByZeroException();
        return lhs / rhs;
    }

    static double solve_right(double target, double lhs) {
        return lhs / target;
    }

    static double solve_left(double target, double rhs) {
        return target * rhs;
    }
};

template <typename C>
struct Op : Expr {
    shared_ptr<Expr> left, right;

    Op() : left(make_shared<Open>()), right(make_shared<Open>()) {}
    Op(shared_ptr<Expr> left, shared_ptr<Expr> right) : left(left), right(right) {}

    virtual bool evaluable() {
        return left->evaluable() && right -> evaluable();
    }

    virtual double evaluate() {
        return C::eval(left->evaluate(), right->evaluate());
    }

    virtual int size() {
        return left->size() + right->size();
    }

    virtual shared_ptr<Expr> fill_left(shared_ptr<Expr> expr, bool &done) {
        return make_shared<Op<C>>(left->fill_left(expr, done), right->fill_left(expr, done));
    }

    virtual double required(double target) {
        // assume only one open node
        if (left->evaluable()) {
            return right->required(C::solve_right(target, left->evaluate()));
        } else {
            return left->required(C::solve_left(target, right->evaluate()));
        }
    }

    virtual set<double> numbers() {
        set<double> result;
        
        for (double number : left->numbers()) result.insert(number);
        for (double number : right->numbers()) result.insert(number);

        return result;
    }

    virtual string to_string() {
        return string() + C::repr + " " + left->to_string() + " " + right->to_string();
    }
};


// literal (a number) in expression tree
struct Lit : Expr {
    double value;

    Lit(double value) : value(value) {}

    virtual bool evaluable() {
        return true;
    }

    virtual double evaluate() {
        return value;
    }

    virtual int size() {
        return 0;
    }

    virtual shared_ptr<Expr> fill_left(shared_ptr<Expr> expr, bool &done) {
        return make_shared<Lit>(value);
    }

    virtual double required(double target) {
        throw RequiredLiteralNodeException();
    }

    virtual set<double> numbers() {
        return set<double>({value});
    }

    virtual string to_string() {
        return std::to_string((int)value);
    }
};


/********************************************************************
BEST EXPRESSION
********************************************************************/

struct Best {
    shared_ptr<Expr> expr;
    double value;

    Best() : expr(make_shared<Open>()), value(0.0) {}
    Best(shared_ptr<Expr> expr) : expr(expr), value(0.0) {
        try {
            value = expr->evaluate();
        } catch (DivisionByZeroException &e) {
            value = 0.0;
        }
    }
};

bool better(const Best &lhs, const Best &rhs, int target) {
    return abs(lhs.value - target) < abs(rhs.value - target);
}

ostream& operator<<(ostream& os, const Best &best) {
    os << best.expr->to_string() << " = " << best.expr->evaluate();
    // os << target << " - " << best.expr->evaluate() << " = " << target-best.expr->evaluate() << endl;
    return os;
}


/********************************************************************
DEPTH FIRST SEARCH
********************************************************************/

Best dfs(shared_ptr<Expr> expr, double target, set<double> numbers, Best best, long long &explored) {
    explored++;

    if (numbers.empty() && expr->evaluable()) {
        // in this case we're on a leaf
        Best current(expr);
        if (better(current, best, target)) best = current;
    } else if (numbers.size() > 0 && !expr->evaluable()) {
        // keep doing recursion
        for (double number : numbers) {
            set<double> next_numbers(numbers);
            next_numbers.erase(number);
            Best opt = dfs(clone_and_fill(expr, make_shared<Lit>(number)), target, next_numbers, best, explored);
            if (better(opt, best, target)) best = opt;

            // lucky stop
            if (best.value == target) return best;
        }

        if (expr->size() < numbers.size()) { // avoid infinite recursion
            Best add = dfs(clone_and_fill(expr, make_shared<Op<Add>>()), target, numbers, best, explored);
            if (better(add, best, target)) best = add;
            if (best.value == target) return best;
            Best sub = dfs(clone_and_fill(expr, make_shared<Op<Sub>>()), target, numbers, best, explored);
            if (better(sub, best, target)) best = sub;
            if (best.value == target) return best;
            Best mul = dfs(clone_and_fill(expr, make_shared<Op<Mul>>()), target, numbers, best, explored);
            if (better(mul, best, target)) best = mul;
            if (best.value == target) return best;
            Best div = dfs(clone_and_fill(expr, make_shared<Op<Div>>()), target, numbers, best, explored);
            if (better(div, best, target)) best = div;
            if (best.value == target) return best;
        }
    }

    return best;
}


/********************************************************************
DEPTH FIRST SEARCH WITH MEMOIZATION
********************************************************************/

Best dfs_mem(shared_ptr<Expr> expr, double target, set<double> numbers, Best best, long long &explored, map<set<double>, map<double, shared_ptr<Expr>>> &mem) {
    explored++;

    if (numbers.empty() && expr->evaluable()) {
        // in this case we're on a leaf
        Best current(expr);
        if (better(current, best, target)) best = current;
    } else if (numbers.size() > 0 && expr->evaluable()) {
        try {
            double outcome = expr->evaluate();
            mem[expr->numbers()][outcome] = expr;
        } catch (DivisionByZeroException &e) {}
    } else if (numbers.size() > 0 && !expr->evaluable()) {
        // keep doing recursion

        // first see if we encountered the missing subtree before
        if (expr->size() == 1) {
            try {
                double required = expr->required(target);
                auto it = mem[numbers].find(required);
                if (it != mem[numbers].end()) {
                    shared_ptr<Expr> answer = clone_and_fill(expr, it->second);
                    return Best(answer);
                }
            } catch (DivisionByZeroException &e) {}
        }

        for (double number : numbers) {
            set<double> next_numbers(numbers);
            next_numbers.erase(number);
            Best opt = dfs_mem(clone_and_fill(expr, make_shared<Lit>(number)), target, next_numbers, best, explored, mem);
            if (better(opt, best, target)) best = opt;

            // lucky stop
            if (best.value == target) return best;
        }

        if (expr->size() < numbers.size()) { // avoid infinite recursion
            Best add = dfs_mem(clone_and_fill(expr, make_shared<Op<Add>>()), target, numbers, best, explored, mem);
            if (better(add, best, target)) best = add;
            if (best.value == target) return best;
            Best sub = dfs_mem(clone_and_fill(expr, make_shared<Op<Sub>>()), target, numbers, best, explored, mem);
            if (better(sub, best, target)) best = sub;
            if (best.value == target) return best;
            Best mul = dfs_mem(clone_and_fill(expr, make_shared<Op<Mul>>()), target, numbers, best, explored, mem);
            if (better(mul, best, target)) best = mul;
            if (best.value == target) return best;
            Best div = dfs_mem(clone_and_fill(expr, make_shared<Op<Div>>()), target, numbers, best, explored, mem);
            if (better(div, best, target)) best = div;
            if (best.value == target) return best;
        }
    }

    return best;
}

/********************************************************************
A* SEARCH
********************************************************************/

struct Node {
    shared_ptr<Expr> expr;
    int dist;
    set<double> numbers;

    Node(shared_ptr<Expr> expr, int dist, set<double> numbers) : expr(expr), dist(dist), numbers(numbers) {}
};

bool operator<(const Node &lhs, const Node &rhs) {
    return lhs.dist < rhs.dist;
}

Best astar(int target, set<double> numbers, long long &explored) {
    Best best;

    priority_queue<Node> q;
    q.emplace(make_shared<Open>(), target, numbers);

    while (!q.empty() && best.value != target) {
        explored++;

        Node cur = q.top();
        q.pop();

        // check for improvement
        if (cur.expr->evaluable() && cur.numbers.empty()) {
            Best opt(cur.expr);
            if (better(opt, best, target)) best = opt;
        }
        // expand children
        else {
            for (double number : cur.numbers) {
                shared_ptr<Expr> expr = clone_and_fill(cur.expr, make_shared<Lit>(number));
                set<double> next_numbers(cur.numbers);
                next_numbers.erase(number);
                q.emplace(expr, target, next_numbers);
            }

            if (cur.expr->size() < cur.numbers.size()) {
                q.emplace(clone_and_fill(cur.expr, make_shared<Op<Add>>()), target, cur.numbers);
            }
        }
    }

    return best;
}

/********************************************************************
MAIN
********************************************************************/

#define MS 1000
#define US 1000000
#define NS 1000000000

struct Metrics {
    Best best;
    long long explored;
    long long s, ms, us, ns;

    Metrics(Best best, long long explored, long long time) : best(best), explored(explored) {
        s = time / NS;
        time -= NS * s;
        ms = time / (NS/MS);
        time -= NS/MS * ms;
        us = time / (NS/US);
        time -= NS/US * us;
        ns = time;
    }
};

ostream& operator<<(ostream &os, const Metrics &metrics) {
    os << "best: " << metrics.best << ", explored " << metrics.explored << " nodes in " << metrics.s 
        << " . " << std::setfill('0') << std::setw(3) << metrics.ms 
        << " " << std::setfill('0') << std::setw(3) << metrics.us 
        << " " << std::setfill('0') << std::setw(3) << metrics.ns << " seconds";
    return os;
}

Metrics run(function<Best(long long&)> task) {
    long long explored = 0;
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();
    Best best = task(explored);
    chrono::steady_clock::time_point end = chrono::steady_clock::now();

    return Metrics(best, explored, chrono::duration_cast<chrono::nanoseconds> (end - begin).count());
}

int main() {
    // double target = 728.0;
    // set<double> numbers = {6.,10.,25.,75.,5.,50.};

    double target = 2500.0;
    set<double> numbers = {1.,2.,3.,4.,5.};

    cout << "target: " << target << endl;
    cout << "numbers: ";
    for (double number : numbers) cout << number << " ";
    cout << endl;

    Metrics m_dfs = run([target, numbers](long long &explored){
        return dfs(make_shared<Open>(), target, numbers, Best(), explored);
    });
    cout << "DFS    " << m_dfs << endl;

    Metrics m_dfs_mem = run([target, numbers](long long &explored){
        map<set<double>, map<double, shared_ptr<Expr>>> mem;
        return dfs_mem(make_shared<Open>(), target, numbers, Best(), explored, mem);
    });
    cout << "DFSMEM " << m_dfs_mem << endl;

    Metrics astar_dfs = run([target, numbers](long long &explored){
        return astar(target, numbers, explored);
    });
    cout << "A*     " << astar_dfs << endl;

    return 0;
}