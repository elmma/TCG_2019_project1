#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include "board.h"
#include "action.h"

class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b) { return action(); }
	virtual bool check_for_win(const board& b) { return false; }

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;
};

class random_agent : public agent {
public:
	random_agent(const std::string& args = "") : agent(args) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
	}
	virtual ~random_agent() {}

protected:
	std::default_random_engine engine;
};

/**
 * random environment
 * add a new random tile to an empty cell
 * (comment add): we should replace by bag mechanism here
 * space : board id spec.  self_role : evil
 * we may need to record the player action
 * 
 * record player action as criterion 
 * note that always add tile to boarder
 * randomly shuffle bag when new bag round
 * to reset the parameter, implement close method
 */
class rndenv : public random_agent {
public:
	rndenv(const std::string& args = "") : random_agent("name=random role=environment " + args),
		space({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }),idx(0) ,bag({1,2,3}) ,popup(0, 2) {}

	virtual void close_episode(const std::string& flag = "") {
		// reset the evil para. for next ep
		idx = 0;
	}

	virtual action take_action(const board& after) {
		std::shuffle(space.begin(), space.end(), engine);
		board::op last = after.get_last_act();	// pass last act
		for (int pos : space) {
			if (after(pos) != 0) continue;

			// boarder: (0,1,2,3) ,(0,4,8,12) ,(12,13,14,15) ,(3,7,11,15)
			if (last == 0 && pos < 12) continue; 
			if (last == 2 && pos > 3) continue; 
			if (last == 1 && pos % 4) continue; 
			if (last == 3 && (pos+1) % 4) continue; 
			
			if(idx == 0 ) std::shuffle(bag.begin(), bag.end(), engine);
			board::cell tile = bag[idx++];
			idx %= 3;

			return action::place(pos, tile);
		}
		return action();
	}

private:
	std::array<int, 16> space;	
	int idx;					// add
	std::array<int, 3> bag;		// add
	std::uniform_int_distribution<int> popup;
};

/**
 * dummy player
 * select a legal action randomly
 *
 * (comment add):we should give it a new heuristic
 * we pick action by choosing the max afterstate value(reward)
 */
class player : public random_agent {
public:
	player(const std::string& args = "") : random_agent("name=dummy role=player " + args),
		opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before) {
		std::shuffle(opcode.begin(), opcode.end(), engine);
		for (int op : opcode) {
			board::reward reward = board(before).slide(op);
			if (reward != -1) return action::slide(op);	
			// in origin , we just pick up a valid slide randomly
		}
		return action();
	}

private:
	std::array<int, 4> opcode;
};
