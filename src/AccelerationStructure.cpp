//
// Created by Danny on 2023-02-08.
//

#include "AccelerationStructure.hpp"

AccelerationStructure::AccelerationStructure(vuk::Context inContext, vuk::Allocator inAllocator): context(std::move(inContext)),
                                                                                              allocator(inAllocator) {

}


void AccelerationStructure::AddBLAS(AccelerationStructure::BlasInput blas_input) {

}