#include "amtModel.h"
#include "utils.h"
#include <iostream>

amtModel::amtModel(): 
    _cqt(),
    _onset_input_cnn("Onset Input"),
    _onset_output_cnn("Onset Output"),
    _note_cnn("Note"),
    _contour_cnn("Contour") {}

void amtModel::reset() {
    _audio_len = 0;
    _Yp_buffer.clear();
    _Yn_buffer.clear();
    _Yo_buffer.clear();
}

std::vector<Note> amtModel::transcribeAudio( const Vectorf& audio ) {

    // reset the model
    reset();

    _audio_len = audio.size();
    auto audio_windowed = getWindowedAudio(audio);
    for ( Vectorf& x : audio_windowed ) {
        inferenceFrame(x);
    }

    // concat 3 buffers
    Matrixf Yp = concatMatrices(_Yp_buffer, _audio_len);
    Matrixf Yn = concatMatrices(_Yn_buffer, _audio_len);
    Matrixf Yo = concatMatrices(_Yo_buffer, _audio_len);

    // convert to midi note events
    return modelOutput2Notes(Yp, Yn, Yo, true);
}

// input shape : (N_AUDIO_SAMPLES, N_BIN_CONTORU )
#include <chrono>

void amtModel::inferenceFrame(const Vectorf& x) {
    VecMatrixf output;

    // Start measuring time for cqtHarmonic
    auto start = std::chrono::high_resolution_clock::now();

    // compute harmonic stacking, shape : (n_harmonics, n_frames, n_bins)
    VecMatrixf cqt = _cqt.cqtHarmonic(x, true);

    // End measuring time for cqtHarmonic
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    // std::cout << duration.count() << std::endl;

    // Start measuring time for other parts
    start = std::chrono::high_resolution_clock::now();

    VecMatrixf contour_out = _contour_cnn.forward(cqt);
    _Yp_buffer.push_back(contour_out[0]); // Yp

    VecMatrixf note_out = _note_cnn.forward(contour_out);
    _Yn_buffer.push_back(note_out[0]); // Yn

    VecMatrixf concat_buf = {note_out[0]};
    VecMatrixf onset_out = _onset_input_cnn.forward(cqt);
    concat_buf.insert(concat_buf.end(), onset_out.begin(), onset_out.end());

    VecMatrixf concat_out = _onset_output_cnn.forward(concat_buf);
    _Yo_buffer.push_back(concat_out[0]); // Yo

    // End measuring time for other parts
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    // std::cout << "Time used for other parts: " << duration.count() << " microseconds" << std::endl;
}

VecMatrixf amtModel::getOutput() {
    // concat 3 buffers
    Matrixf Yp = concatMatrices(_Yp_buffer, _audio_len);
    Matrixf Yn = concatMatrices(_Yn_buffer, _audio_len);
    Matrixf Yo = concatMatrices(_Yo_buffer, _audio_len);
    return {Yp, Yn, Yo};
}