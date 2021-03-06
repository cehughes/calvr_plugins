#ifndef FUTURE_PATIENT_POINT_ACTIONS_H
#define FUTURE_PATIENT_POINT_ACTIONS_H

#include <string>

class PointAction
{
    public:
        virtual void action()=0;
};

class PointActionPDF : public PointAction
{
    public:
        PointActionPDF(std::string file);
        virtual void action();

    protected:
        std::string _file;
};

#endif
