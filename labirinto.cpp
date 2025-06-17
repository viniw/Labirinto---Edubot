/*Código do Labirinto - Mapa Maze - Metodologia: Tremaux*/

#include <iostream>
#include <map>
#include <tuple>
#include <vector>
#include "libs/EdubotLib.hpp"

enum Direction { NORTH = 0, EAST = 90, SOUTH = 180, WEST = 270 };

struct State {
    int x, y, dir;
    bool operator<(const State& other) const {
        return std::tie(x, y, dir) < std::tie(other.x, other.y, other.dir);
    }
};

int normalize(int angle) {
    angle %= 360;
    if (angle < 0) angle += 360;
    return angle;
}

void rotateTo(EdubotLib* edubot, int relativeAngle) {
    edubot->rotate(relativeAngle);
    edubot->sleepMilliseconds(900);
}

void moveForward(EdubotLib* edubot, double speed) {
    edubot->move(speed);
    edubot->sleepMilliseconds(1000); // percorre cerca de 0.14 m
    edubot->stop();
}

int main() {
    EdubotLib* edubot = new EdubotLib();

    if (!edubot->connect()) {
        std::cout << "Erro ao conectar no Edubot." << std::endl;
        return -1;
    }

    double safeDistance = 0.25;
    double speed = 0.14;
    int currentAngle = NORTH;
    std::map<State, int> visitCount;

    int openCycles = 0;

    while (true) {
        double sonar[7];
        for (int i = 0; i < 7; i++)
            sonar[i] = edubot->getSonar(i);

        int openSensors = 0;
        for (int i = 0; i < 7; i++) {
            if (sonar[i] > 1.0) openSensors++;
        }

        if (openSensors >= 4) {
            openCycles++;
        } else {
            openCycles = 0;
        }

        if (openCycles >= 3) {
            std::cout << "Saída confirmada do labirinto com 4 sensores abertos. Parando o robô." << std::endl;
            edubot->stop();
            break;
        }

        bool bumper = edubot->getBumper(0) || edubot->getBumper(1) ||
                      edubot->getBumper(2) || edubot->getBumper(3);

        if (bumper) {
            edubot->stop();
            edubot->move(-0.1);
            edubot->sleepMilliseconds(700);
            edubot->stop();
            rotateTo(edubot, -45);
            currentAngle = normalize(currentAngle - 45);
            continue;
        }

        int x = static_cast<int>(edubot->getX() * 10);
        int y = static_cast<int>(edubot->getY() * 10);
        State currentState = {x, y, currentAngle};
        visitCount[currentState]++;

        double front = sonar[3];
        double frontLeft = sonar[2];
        double frontRight = sonar[4];
        double left = sonar[1];
        double right = sonar[5];

        struct Option {
            int angleOffset;
            double clearance;
            int visits;
        };
        std::vector<Option> options;

        auto tryOption = [&](int offset, double clearance, bool condition) {
            if (condition) {
                int dir = normalize(currentAngle + offset);
                int dx = 0, dy = 0;
                if (dir == NORTH) dy += 1;
                else if (dir == SOUTH) dy -= 1;
                else if (dir == EAST)  dx += 1;
                else if (dir == WEST)  dx -= 1;
                State nextState = {x + dx, y + dy, dir};
                int v = visitCount[nextState];
                if (offset != 0) v += 1;  // penalidade leve por virar
                options.push_back({offset, clearance, v});
            }
        };

        tryOption(0, front, front > safeDistance && frontLeft > safeDistance && frontRight > safeDistance);
        tryOption(-45, left, left > safeDistance && frontLeft > safeDistance);
        tryOption(45, right, right > safeDistance && frontRight > safeDistance);

        Option best = {180, 0, 9999};

        // Primeira prioridade: caminho nunca visitado
        for (auto& opt : options) {
            if (opt.visits == 0) {
                best = opt;
                break;
            }
        }

        // Se todos já foram visitados, escolhe o menos visitado
        if (best.visits == 9999) {
            for (auto& opt : options) {
                if (opt.visits < best.visits)
                    best = opt;
            }
        }

        rotateTo(edubot, best.angleOffset);
        currentAngle = normalize(currentAngle + best.angleOffset);
        if (best.angleOffset != 180) {
            moveForward(edubot, speed);
        }

        edubot->sleepMilliseconds(100);
    }

    edubot->disconnect();
    return 0;
}
