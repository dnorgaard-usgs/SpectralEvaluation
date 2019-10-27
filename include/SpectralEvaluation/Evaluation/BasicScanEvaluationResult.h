#pragma once

#include <vector>
#include "EvaluationResult.h"
#include "../Spectra/SpectrumInfo.h"

namespace Evaluation
{
    /** An instance of the class BasicScanEvaluationResult contains a minimal amount of information
        regarding the evaluation of a full scan using one fit-window.
        This forms the base-class for the classes CScanResult in NovacProgram and CScanResult in NovacPPP. */
    class BasicScanEvaluationResult
    {
    public:

        /** The results of evaluating the spectra.
            There is one evaluation result for each spectrum in the scan.
            These are ordered such that m_spec[i] contains the result for spectrum #i in the scan. */
        std::vector<CEvaluationResult> m_spec;

        /** General information about the collected spectra.
            There is one entry here for each spectrum in the scan, and element #i must match 
             element #i in the vector m_spec. */
        std::vector<CSpectrumInfo> m_specInfo;

        /** information about the sky-spectrum used */
        CSpectrumInfo m_skySpecInfo;

        /** information about the dark-spectrum used, if any */
        CSpectrumInfo m_darkSpecInfo;

        /** information about the offset-spectrum used, if any */
        CSpectrumInfo m_offsetSpecInfo;

        /** information about the dark-current spectrum used, if any */
        CSpectrumInfo m_darkCurSpecInfo;

        /** A path where the .pak file associated with this result resides.
            This makes it possible to re-read the scan and analyze it again. */
        std::string m_path = "";

        /** A list of which spectra were corrupted and could not be evaluated.
            There is one entry here for each spectrum which wasn't evaluated. */
        std::vector<unsigned int> m_corruptedSpectra;

        /** Returns the index of the specie with the provided name.
            E.g. checks the scan result for which index corresponds to SO2.
            The comparison is done ignoring case.
            @return -1 if the specie could not be found */
        int GetSpecieIndex(const char* specieName) const;
    };
}