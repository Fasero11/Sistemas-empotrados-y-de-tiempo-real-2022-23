import matplotlib.pyplot as plt
import pandas as pd

dataS1 = pd.read_csv('./Lab_NoRT/cyclictestURJC_S1.csv') # IDLE
df1 = pd.DataFrame(dataS1)

dataS2 = pd.read_csv('./Lab_NoRT/cyclictestURJC_S2.csv') # HACKBENCH
df2 = pd.DataFrame(dataS2)

dataS3 = pd.read_csv('./Lab_NoRT/cyclictestURJC_S3.csv') # BONNIE
df3 = pd.DataFrame(dataS3)

X1 = list(df1.iloc[:, 2]/1000)
X2 = list(df2.iloc[:, 2]/1000)
X3 = list(df3.iloc[:, 2]/1000)

avg1 = sum(list(df1.iloc[:, 2])) / len(list(df1.iloc[:, 2]))
max1 = max(list(df1.iloc[:, 2]))
print("avg: ", avg1, "max: ", max1)

avg2 = sum(list(df2.iloc[:, 2])) / len(list(df2.iloc[:, 2]))
max2 = max(list(df2.iloc[:, 2]))
print("avg: ", avg2, "max: ", max2)

avg3 = sum(list(df3.iloc[:, 2])) / len(list(df3.iloc[:, 2]))
max3 = max(list(df3.iloc[:, 2]))
print("avg: ", avg3, "max: ", max3)

# Plot the data using bar() method
plt.hist(X1, range=[0,55], edgecolor='black', linewidth=0.33, bins=100, alpha=0.5, label="S1: Idle")
plt.hist(X2, range=[0,55], edgecolor='black', linewidth=0.33, bins=100, alpha=0.5, label="S2: Hackbench")
plt.hist(X3, range=[0,55], edgecolor='black', linewidth=0.33, bins=100, alpha=0.5, label="S3: Bonnie++")
plt.legend(loc="upper right")
plt.title("Laboratiorios Kernel NO RT")
plt.xlabel("latencia (microsegundos)")
plt.ylabel("frecuencia")

# Show the plot
plt.show()