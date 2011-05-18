/*
 * CompactTriples.cpp
 *
 *  Created on: 11/05/2011
 *      Author: mck
 */

#include "CompactTriples.hpp"
#include "TripleOrderConvert.hpp"

namespace hdt {


CompactTriples::CompactTriples() : numTriples(0), order(SPO) {
	masterStream = new UintStream();
	slaveStream = new UintStream();
}

CompactTriples::~CompactTriples() {
	delete masterStream;
	delete slaveStream;
}


float CompactTriples::cost(TripleID & triple)
{
}



void CompactTriples::load(ModifiableTriples &triples) {
	TripleID all(0,0,0);

	IteratorTripleID *it = triples.search(all);

	vector<unsigned int> vectorY, vectorZ;
	unsigned int lastX, lastY, lastZ;
	unsigned int x, y, z;

	// First triple
	if(it->hasNext()) {
		TripleID triple = it->next();
		//cout << "111> " << triple << endl;

		UnorderedTriple *ut = reinterpret_cast<UnorderedTriple *>(&triple);
		swapComponentOrder(ut, SPO, order);

		lastX = x = ut->x;
		lastY = y = ut->y;
		lastZ = z = ut->z;

		vectorY.push_back(y);
		vectorZ.push_back(z);

		numTriples++;
	}

	// Rest of the triples
	while(it->hasNext()) {
		TripleID triple = it->next();
		//cout << "111> " << triple << endl;

		UnorderedTriple *ut = reinterpret_cast<UnorderedTriple *>(&triple);
		swapComponentOrder(ut, SPO, order);

		x = ut->x;
		y = ut->y;
		z = ut->z;

		if(x!=lastX) {
			vectorY.push_back(0);
			vectorY.push_back(y);

			vectorZ.push_back(0);
			vectorZ.push_back(z);
		} else if(y!=lastY) {
			vectorY.push_back(y);
			vectorZ.push_back(0);
			vectorZ.push_back(z);
		} else {
			vectorZ.push_back(z);
		}

		lastX = x;
		lastY = y;
		lastZ = z;

		numTriples++;
	}

	delete it;

	VectorIterator itY(vectorY);
	VectorIterator itZ(vectorZ);

	masterStream->add(itY);
	slaveStream->add(itZ);

#if 0
	// Debug Adjacency Lists
	cout << "Y" << vectorY.size() << "): ";
	for(unsigned int i=0;i<masterStream->getNumberOfElements();i++){
		cout << masterStream->get(i) << " ";
	}
	cout << endl;

	cout << "Z" << vectorZ.size() << "): ";
	for(unsigned int i=0;i<slaveStream->getNumberOfElements();i++){
		cout << slaveStream->get(i) << " ";
	}
	cout << endl;
#endif

}

void CompactTriples::populateHeader(Header &header) {

}

IteratorTripleID *CompactTriples::search(TripleID & pattern)
{
	return new CompactTriplesIterator(this, pattern);
}

bool CompactTriples::save(std::ostream & output, ControlInformation &controlInformation)
{
	controlInformation.clear();
	controlInformation.setUint("numTriples", getNumberOfElements());
	controlInformation.set("codification", "http://purl.org/HDT/hdt#triplesCompact");
	controlInformation.setUint("componentOrder", order);
	controlInformation.save(output);

	masterStream->save(output);
	slaveStream->save(output);
}

void CompactTriples::load(std::istream &input, ControlInformation &controlInformation)
{
	numTriples = controlInformation.getUint("numTriples");
	order = (TripleComponentOrder) controlInformation.getUint("componentOrder");

	masterStream->load(input);
	slaveStream->load(input);
}

unsigned int CompactTriples::getNumberOfElements()
{
	return numTriples;
}

unsigned int CompactTriples::size()
{
	return masterStream->size()+slaveStream->size();
}

/// ITERATOR
CompactTriplesIterator::CompactTriplesIterator(CompactTriples *pt, TripleID &pat)
		: triples(pt), numTriple(0), masterPos(0), slavePos(0), pattern(pat) {

	doFetch();
}

CompactTriplesIterator::~CompactTriplesIterator() {

}

void CompactTriplesIterator::readTriple() {
	if(numTriple==0) {
		x = 1;
		y = triples->masterStream->get(masterPos++);
		z = triples->slaveStream->get(slavePos++);
	} else {
		z = triples->slaveStream->get(slavePos++);

		if(z==0) {
			z = triples->slaveStream->get(slavePos++);

			y = triples->masterStream->get(masterPos++);

			if(y==0) {
				y = triples->masterStream->get(masterPos++);
				x++;
			}
		}
	}

	//cout << numTriple << "/" << triples->numTriples << "  "<< x << ", " << y << ", " << z << endl;

	nextv.setSubject(x);
	nextv.setPredicate(y);
	nextv.setObject(z);

	UnorderedTriple *trip = reinterpret_cast<UnorderedTriple *>(&nextv);
	swapComponentOrder(trip, triples->order, SPO);

	numTriple++;

	hasNextv = (numTriple<=triples->numTriples);
}

void CompactTriplesIterator::doFetch() {
	do {
		readTriple();
	} while(hasNextv && (!nextv.isValid() || !nextv.match(pattern)));
}

bool CompactTriplesIterator::hasNext() {
	return hasNextv;
}

TripleID CompactTriplesIterator::next() {
	TripleID ret = nextv;
	doFetch();
	return ret;
}


}
