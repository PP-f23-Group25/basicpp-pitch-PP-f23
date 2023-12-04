#pragma once

#include "typedef.h"
#include "layer.h"
#include <vector>
#include <string>


class CNN {
    public:

        CNN( const std::string model_name );

        ~CNN();
    
        // inference API for Eigen IO
        Tensor3f forward( const Tensor3f& input ) const;

        std::string get_name() const;
    
        std::vector<Layer*> _layers;

    private:
    
        std::string _model_name;
};