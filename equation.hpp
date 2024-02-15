#include <iostream>
#include <math.h>
#include <vector>
#include <fstream>
#include <string>
#include <stack>

class Equation {
public:
	int precedence(char c) {
		if (c == '+' || c == '-')
			return 1;
		else if (c == '*' || c == '/')
			return 2;
		else if (c == '^')
			return 3;
		else
			return -1;
	}

	bool isUnaryMinus(char c, size_t pos, const std::string& infix) {
		return c == '-' && (pos == 0 || infix[pos - 1] == '(' || isOperator(infix[pos - 1]));
	}

	bool isOperator(char c) {
		return c == '+' || c == '-' || c == '*' || c == '/' || c == '^';
	}

	std::vector<std::string> infixToPostfix(std::string infix) {
		std::stack<std::string> st;
		std::vector<std::string> postfix;
		std::string token;
		for (int i = 0; i < infix.size(); i++) {
			char c = infix[i];
			if (std::isdigit(c) || c == '.' || std::isalpha(c)) {
				token += c;
			}
			else {
				if (!token.empty()) {
					postfix.push_back(token);
					token.clear();
				}
				if (c == '(') {
					st.push(std::string(1, c));
				}
				else if (c == ')') {
					while (!st.empty() && st.top() != "(") {
						postfix.push_back(st.top());
						st.pop();
					}
					st.pop();
				}
				else if (isOperator(c)) {
					while (!st.empty() && st.top() != "(" && precedence(c) <= precedence(st.top()[0])) {
						postfix.push_back(st.top());
						st.pop();
					}
					st.push(std::string(1, c));
				}
				else if (isUnaryMinus(c, i, infix)) {
					token += '-';
				}
			}
		}
		if (!token.empty()) {
			postfix.push_back(token);
		}
		while (!st.empty()) {
			postfix.push_back(st.top());
			st.pop();
		}
		return postfix;
	}

	double calculatePostfix(std::vector<std::string> postfix) {
		std::stack<double> opstack;

		for (int i = 0; i < postfix.size(); i++) {
			std::string num = postfix[i];
			try {
				if (num.length() == 1 && (num[0] == '+' || num[0] == '-' || num[0] == '*' || num[0] == '/' || num[0] == '^')) {
					if (opstack.size() < 2)
						throw std::runtime_error("Not enough operands: " + num);

					double second = opstack.top();
					opstack.pop();
					double first = opstack.top();
					opstack.pop();

					if (num[0] == '+') {
						double temp = first + second;
						opstack.push(temp);
					}
					else if (num[0] == '-') {
						double temp = first - second;
						opstack.push(temp);
					}
					else if (num[0] == '*') {
						double temp = first * second;
						opstack.push(temp);
					}
					else if (num[0] == '/') {
						if (second == 0) {
							throw std::runtime_error("Division by zero");
						}
						else {
							double temp = first / second;
							opstack.push(temp);
						}
					}
					else if (num[0] == '^') {
						double temp = std::pow(first, second);
						opstack.push(temp);
					}
				}
				else {
					double number = std::stod(num);
					opstack.push(number);
				}
			}
			catch (const std::exception& e) {
				//std::cout << "Error" << std::endl;
			}
		}
		if (opstack.size() != 1)
			std::cout << "Invalid postfix expression" << std::endl;
		return opstack.top();
	}

	double evaluate(std::string expression) {
		std::vector<std::string> postfix = infixToPostfix(expression);
		return calculatePostfix(postfix);
	}

	double evaluateWithXY(std::string expression, double x, double y) {
		size_t xPos = expression.find("x");
		while (xPos != std::string::npos) {
			expression.replace(xPos, 1, "(" + std::to_string(x) + ")");
			xPos = expression.find("x", xPos + 1);
		}

		size_t yPos = expression.find("y");
		while (yPos != std::string::npos) {
			expression.replace(yPos, 1, "(" + std::to_string(y) + ")");
			yPos = expression.find("y", yPos + 1);
		}

		return evaluate(expression);
	}
};