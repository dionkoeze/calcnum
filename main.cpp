#include <iostream>
#include <string>
#include <memory>
#include <set>
#include <chrono>

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

/********************************************************************
EXPRESSION TREES
********************************************************************/

// expression tree interface
struct Expr {
    virtual bool evaluable() =0;
    virtual int evaluate() =0;
    virtual int size() =0;
    virtual shared_ptr<Expr> fill_left(shared_ptr<Expr> expr, bool &done) =0;

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

    virtual int evaluate() {
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

    virtual string to_string() {
        return ".";
    }
};

// operator in expression tree
struct Add {
    static const char repr = '+';

    static int eval(int lhs, int rhs) {
        return lhs + rhs;
    }
};

struct Sub {
    static const char repr = '-';

    static int eval(int lhs, int rhs) {
        return lhs - rhs;
    }
};

struct Mul {
    static const char repr = '*';

    static int eval(int lhs, int rhs) {
        return lhs * rhs;
    }
};

struct Div {
    static const char repr = '/';

    static int eval(int lhs, int rhs) {
        if (rhs == 0) throw DivisionByZeroException();
        return lhs / rhs;
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

    virtual int evaluate() {
        return C::eval(left->evaluate(), right->evaluate());
    }

    virtual int size() {
        return left->size() + right->size();
    }

    virtual shared_ptr<Expr> fill_left(shared_ptr<Expr> expr, bool &done) {
        return make_shared<Op<C>>(left->fill_left(expr, done), right->fill_left(expr, done));
    }

    virtual string to_string() {
        return string() + C::repr + " " + left->to_string() + " " + right->to_string();
    }
};


// literal (a number) in expression tree
struct Lit : Expr {
    int value;

    Lit(int value) : value(value) {}

    virtual bool evaluable() {
        return true;
    }

    virtual int evaluate() {
        return value;
    }

    virtual int size() {
        return 0;
    }

    virtual shared_ptr<Expr> fill_left(shared_ptr<Expr> expr, bool &done) {
        return make_shared<Lit>(value);
    }

    virtual string to_string() {
        return std::to_string(value);
    }
};


/********************************************************************
SEARCH TREE
********************************************************************/

struct Best {
    shared_ptr<Expr> expr;
    int value;

    Best() : expr(make_shared<Open>()), value(0) {}
    Best(shared_ptr<Expr> expr) : expr(expr), value(0) {
        try {
            value = expr->evaluate();
        } catch (DivisionByZeroException &e) {
            value = 0;
        }
    }
};

bool better(const Best &lhs, const Best &rhs, int target) {
    return abs(lhs.value - target) < abs(rhs.value - target);
}


Best find(shared_ptr<Expr> expr, int target, set<int> numbers, Best best, long long &explored) {
    explored++;

    if (numbers.size() == 0 && expr->evaluable()) {
        // in this case we're on a leaf
        Best current(expr);
        if (better(current, best, target)) best = current;
    } else if (numbers.size() > 0 && !expr->evaluable()) {
        // keep doing recursion
        for (int number : numbers) {
            set<int> next_numbers(numbers);
            next_numbers.erase(number);
            Best opt = find(clone_and_fill(expr, make_shared<Lit>(number)), target, next_numbers, best, explored);
            if (better(opt, best, target)) best = opt;

            // lucky stop
            if (best.value == target) return best;
        }

        if (expr->size() < numbers.size()) { // avoid infinite recursion
            Best add = find(clone_and_fill(expr, make_shared<Op<Add>>()), target, numbers, best, explored);
            if (better(add, best, target)) best = add;
            if (best.value == target) return best;
            Best sub = find(clone_and_fill(expr, make_shared<Op<Sub>>()), target, numbers, best, explored);
            if (better(sub, best, target)) best = sub;
            if (best.value == target) return best;
            Best mul = find(clone_and_fill(expr, make_shared<Op<Mul>>()), target, numbers, best, explored);
            if (better(mul, best, target)) best = mul;
            if (best.value == target) return best;
            Best div = find(clone_and_fill(expr, make_shared<Op<Div>>()), target, numbers, best, explored);
            if (better(div, best, target)) best = div;
            if (best.value == target) return best;
        }
    }

    return best;
}


/********************************************************************
MAIN
********************************************************************/

int main() {
    int target = 25;
    set<int> numbers = {1,2,3,4,5};

    long long explored = 0;

    chrono::steady_clock::time_point begin = chrono::steady_clock::now();
    Best best = find(make_shared<Open>(), target, numbers, Best(), explored);
    chrono::steady_clock::time_point end = chrono::steady_clock::now();

    cout << "target: " << target << endl;
    cout << "numbers: ";
    for (int number : numbers) cout << number << " ";
    cout << endl;
    
    cout << "best: " << best.expr->to_string() << endl;
    cout << target << " - " << best.expr->evaluate() << " = " << target-best.expr->evaluate() << endl;

    cout << "explored: " << explored << endl;
    cout << chrono::duration_cast<chrono::seconds>(end - begin).count() << " [s]" << endl;
    cout << chrono::duration_cast<chrono::milliseconds>(end - begin).count() << " [ms]" << endl;
    cout << chrono::duration_cast<chrono::microseconds>(end - begin).count() << " [µs]" << endl;
    cout << chrono::duration_cast<chrono::nanoseconds> (end - begin).count() << " [ns]" << endl;
    return 0;
}