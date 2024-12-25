data = {}

header_strs = "tick_us	cmd_s	act_s	is_drilling	kn_detected	breakout_detected	detect_started	detect_state	realtime_voltage	averaged_voltage	kn	kn_valid_rate	kn_cnt"

headers = header_strs.split('\t')
print(headers)

for header in headers:
    data[header] = []

# Read the text file
with open('Data/MotionRecordData/Decode/Data2_20241224_11_20_48_745.txt', 'r') as file:
    lines = file.readlines()
    print(len(lines))

# Process each line
for line in lines:
    # Split the line into columns
    columns = line.strip().split('\t')
    
    # Store each column in an array
    for i, column in enumerate(columns):
        data[headers[i]].append(column)
        
print(len(data))
print(len(data[headers[0]]))

import matplotlib.pyplot as plt

# Convert tick_us to seconds
tick_us = [int(tick) / 1000000 for tick in data['tick_us']]

# Convert cmd_s to float
cmd_s = [float(cmd) for cmd in data['cmd_s']]

# Convert averaged_voltage to float
averaged_voltage = [float(voltage) for voltage in data['averaged_voltage']]

# Create the figure and axis objects
fig, ax1 = plt.subplots()

# Plot the first y-axis data
ax1.plot(tick_us, cmd_s, color='blue')
ax1.set_xlabel('Time (s)')
ax1.set_ylabel('cmd_s', color='blue')

# Create a second y-axis
ax2 = ax1.twinx()

# Plot the second y-axis data
ax2.plot(tick_us, averaged_voltage, color='red')
ax2.set_ylabel('averaged_voltage', color='red')

# Show the plot
plt.show()