#include <iostream>
#include <memory>

using namespace std;

// expression tree interface
struct Expr {
    virtual bool evaluable() =0;
    virtual int evaluate() =0;
};

struct OpenNodeEvalException : public exception {
    virtual const char* what() const throw() {
        return "cannot evaluate expression tree with open node";
    }
};

// open node in expression tree
struct Open : Expr {
    Open() {}

    virtual bool evaluable() {
        return false;
    }

    virtual int evaluate() {
        throw new OpenNodeEvalException();
    }
};

// operator in expression tree
struct Op : Expr {
    shared_ptr<Expr> left, right;

    Op(Expr *left, Expr *right) : left(left), right(right) {}

    virtual bool evaluable() {
        return left->evaluable() && right->evaluable();
    }
};

// all four operators
struct Add : Op {
    Add(Expr *left, Expr *right) : Op(left, right) {}

    virtual int evaluate() {
        return left->evaluate() + right->evaluate();
    }
};

struct Sub : Op {
    Sub(Expr *left, Expr *right) : Op(left, right) {}

    virtual int evaluate() {
        return left->evaluate() - right->evaluate();
    }
};

struct Mul : Op {
    Mul(Expr *left, Expr *right) : Op(left, right) {}

    virtual int evaluate() {
        return left->evaluate() * right->evaluate();
    }
};

struct Div : Op {
    Div(Expr *left, Expr *right) : Op(left, right) {}

    virtual int evaluate() {
        return left->evaluate() / right->evaluate();
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
};

int main() {
    Expr *rootA = new Add(new Lit(3), new Open());
    Expr *rootB = new Add(new Lit(3), new Lit(6));

    cout << rootA->evaluable() << endl;
    cout << rootB->evaluable() << endl;
    cout << rootB->evaluate() << endl;
    cout << rootA->evaluate() << endl;

    return 0;
}