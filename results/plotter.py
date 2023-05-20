import pandas as pd
import matplotlib.pyplot as plt

data_file = 'results-sender-1Gb.result'
skip_test_names = ['empms']
#skip_test_names = []

# Read the CSV data into a Pandas dataframe
df = pd.read_csv('results-sender-1Gb.result')
print(df)
for skip_test_name in skip_test_names:
    df = df.drop(df[ df['test name'] == skip_test_name].index)
print(df)

# Group the data by test name and number of bytes, and calculate the mean wall clock time
grouped_df = df.groupby(['test name', 'number of bytes']).mean(numeric_only=True).reset_index()
print(grouped_df)

# Get a list of all unique test names and number of bytess
test_names = grouped_df['test name'].unique()
data_sizes = grouped_df['number of bytes'].unique()

# Set up the plot
fig, ax = plt.subplots()

# Set the width of each bar and the spacing between groups of bars
bar_width = 0.2
spacing = 0.01

for i, test_name in enumerate(test_names):
    # Create a series of wall clock times for the current test name
    test_series = grouped_df[grouped_df['test name'] == test_name][['wall clock us', 'number of bytes']]
    print(test_series)

    # Calculate the x positions for the bars in the current group
    x_positions = [ i * (bar_width + spacing) + j for j in range(len(data_sizes))]

    # Create the bars for the current group
    ax.bar(x_positions, test_series['wall clock us'], width=bar_width, label=test_name)

# Set the x ticks and labels for each group of bars
ax.set_xticks([i + (len(data_sizes) - 1) / 2 * (bar_width + spacing) for i in range(len(data_sizes))])
ax.set_xticklabels(data_sizes)

# Add labels and legend
ax.set_xlabel('number of bytes')
ax.set_ylabel('Wall Clock Time (us)')
ax.legend()

plt.show()

