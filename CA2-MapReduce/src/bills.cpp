#include "../lib/bills.hpp"

void Bills::read_coeffs() {
    io::CSVReader<5> in(path);
    in.read_header(io::ignore_extra_column, "Year", "Month", "water", "gas", "electricity");

    int year, month, v1, v2, v3;
    int i = 0;
    while(in.read_row(year, month, v1, v2, v3)) {
        resources_coeffs[i] = new ResourceCoefficient(year, month, v1, v2, v3);
        i++;
    }
}

void Bills::save_records(char* encoded_records) {
    records = RecordSerializer::decode(encoded_records);
}

double Bills::calculate_bill(ResourceType source, int month) {
    switch (source) {
        case (ResourceType::GAS):
            return calculate_gas_bill(month);
            break;
        case (ResourceType::ELEC):
            return calculate_elec_bill(month);
            break;
        case (ResourceType::WATER):
            return calculate_water_bill(month);
            break;
        
        default:
            throw std::invalid_argument("Invalid source type");
            break;
    }
}

int Bills::get_coeff(int month) {
    for (const ResourceCoefficient* row : resources_coeffs)
        if (row->month == month) {
            return row->coeffs[GAS_COEFF_INDEX];
            break;
        }
    throw std::runtime_error("Invalid month");
    return -1;
}

double Bills::calculate_gas_bill(int month) {
    double cost = 0;

    for (const Record* record : records)
        if (record->month == month)
            for (const int usage : record->usages)
                cost += usage;

    return get_coeff(month) * cost;
}

double Bills::calculate_water_bill(int month) {
    double cost = 0;
    int max_hour = util_calculate_max_usage_hour(records, month);
    for (const Record* record : records)
        if (record->month == month)
            for (int hour = 0; hour < record->usages.size(); hour++)
                if (hour == max_hour)
                    cost += record->usages[hour] * WATER_OVERUSE_COEFF;
                else
                    cost += record->usages[hour];

    
    return get_coeff(month) * cost;
}

double Bills::calculate_elec_bill(int month) {
    double cost = 0;
    int max_hour = util_calculate_max_usage_hour(records, month);
    double avg = util_calculate_avg_usage(records, month);

    for (const Record* record : records)
        if (record->month == month)
            for (int hour = 0; hour < record->usages.size(); hour++)
                if (hour == max_hour)
                    cost += record->usages[hour] * WATER_OVERUSE_COEFF;
                else if (avg < record->usages[hour])
                    cost += record->usages[hour] * ELEC_UNDERUSE_COEFF;
                else
                    cost += record->usages[hour];
    
    return get_coeff(month) * cost;
}