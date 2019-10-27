#pragma once
#include <vector>

class CPlumeInScanProperty;

namespace FileHandler
{
    class CScanFileHandler;
}

namespace Evaluation
{
    class BasicScanEvaluationResult;

    // This is a helper class for selecting 'in plume' and 'out of plume' 
    //  (based on some basic selection criteria) spectra with 
    //  the intention of creating reference spectra for performing e.g. 
    //  ratio evaluations or detailed spectral analysis of compounds in the plume.
    class PlumeSpectrumSelector
    {
    public:
        // TODO: setup a proper set of values here!
        struct PlumeSpectrumSelectionSettings
        {
            int minNumberOfSpectraInPlume = 7;

            int minNumberOfSpectraOutsideOfPlume = 10;

            // The minimum (SO2) column for the selected spectra in the plume.
            double minInPlumeColumn = 1e17;

            // The maximum saturation ratio (intensity / maximum intensity of spectrometer)
            double maxSaturationRatio = 0.88;

            // The maximum saturation ratio (intensity / maximum intensity of spectrometer)
            double minSaturationRatio = 0.12;
        };

        void CreatePlumeSpectrumFile(
            FileHandler::CScanFileHandler& originalScanFile,
            const BasicScanEvaluationResult& scanResult,
            const CPlumeInScanProperty& properties,
            int mainSpecieIndex,
            const std::string& outputDirectory);

    private:
        static bool IsSuitableScanForRatioEvaluation(
            FileHandler::CScanFileHandler& scanFile,
            const PlumeSpectrumSelectionSettings& settings,
            const BasicScanEvaluationResult& scanResult,
            const CPlumeInScanProperty& properties);

        // Checks the provided evaluated scan using default settings 
        //  and returns the indices of the spectra which can 
        //  be saved as in-plume and out-of-plume spectra.
        // This will use the provided CPlumeInScanProperty to find out
        //   if the scan sees the plume at all and where the edges of 
        //   the plume are located.
        // If this check fails, then the two vectors are empty.
        static void SelectSpectra(
            const PlumeSpectrumSelectionSettings& settings,
            FileHandler::CScanFileHandler& scanFile,
            const BasicScanEvaluationResult& scanResult,
            const CPlumeInScanProperty& properties,
            int mainSpecieIndex,
            std::vector<size_t>& referenceSpectra,
            std::vector<size_t>& inPlumeSpectra);

        // Calculates the average column value of the given specie in the index [startIdx, endIdx[
        static double AverageColumnValue(
            const BasicScanEvaluationResult& scanResult,
            int specieIndex,
            size_t startIdx,
            size_t endIdx);

        static std::vector<size_t> FindSpectraInPlume(
            const BasicScanEvaluationResult& scanResult, 
            const CPlumeInScanProperty& properties,
            int mainSpecieIndex,
            const PlumeSpectrumSelectionSettings& settings);

        static std::vector<size_t> FilterSpectraUsingIntensity(
            const std::vector<size_t>& proposedIndices,
            FileHandler::CScanFileHandler& scanFile,
            const PlumeSpectrumSelectionSettings& settings);

    };
}
