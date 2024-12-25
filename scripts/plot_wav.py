import numpy as np
from scipy.io import wavfile
from scipy.signal import spectrogram
import scipy
import scipy.signal

import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap
import os
import tkinter as tk
from tkinter import filedialog

def get_adjusted_inferno_cmap(split_point_1 = 90, split_point_2 = 200):
    # 获取 inferno colormap 对象
    inferno_cmap = plt.get_cmap('inferno')

    # 提取 inferno colormap 的颜色数据
    inferno_colors = inferno_cmap(np.linspace(0, 1, inferno_cmap.N))
    
    # 将原先 0-split_point_1 的数据插值映射到 0-split_point_2 中
    new_colors_0_split_point_2 = np.zeros((split_point_2 + 1, 4))
    for i in range(4):
        new_colors_0_split_point_2[:, i] = np.interp(np.linspace(0, split_point_1, split_point_2 + 1), np.arange(split_point_1 + 1), inferno_colors[:split_point_1 + 1, i])

    # 将原先 split_point_1+1-255 的数据插值映射到 split_point_2+1-255 中
    new_colors_split_point_2_255 = np.zeros((256 - split_point_2 - 1, 4))
    for i in range(4):
        new_colors_split_point_2_255[:, i] = np.interp(np.linspace(split_point_1 + 1, 255, 256 - split_point_2 - 1), np.arange(split_point_1 + 1, 256), inferno_colors[split_point_1 + 1:, i])

    # 合并新的颜色数据
    inferno_colors[:split_point_2 + 1] = new_colors_0_split_point_2
    inferno_colors[split_point_2 + 1:] = new_colors_split_point_2_255

    # 将修改后的颜色数据转换为新的 colormap 对象
    new_inferno_cmap = LinearSegmentedColormap.from_list('new_inferno', inferno_colors, inferno_colors.shape[0])
    
    return new_inferno_cmap

def process_file(filename: str, save_file: bool=False, show_pic: bool=True, folder='output5'):
    # 读取音频文件
    sample_rate, data = wavfile.read(filename)
    print("sample_rate: ", sample_rate)
    print("data shape: ", data.shape)

    # 绘制原始数据和分贝数据
    time = np.arange(data.shape[0]) / sample_rate

    plt.figure(figsize=(12, 6))
    
    subplotnum = 3
    
    def add_grid_ticks():
        plt.xticks(np.arange(0, time[-1], 1))
        plt.grid(True, which='both', axis='x', linestyle='--', linewidth=0.5)

    plt.subplot(subplotnum, 1, 1)
    plt.plot(time, data, linewidth=0.1)
    plt.title('Original Audio Data')
    plt.xlabel('Time [s]')
    plt.ylabel('Amplitude')
    plt.xlim([time[0], time[-1]])

    nfft = 1024
    noverlap = 512
    plt.subplot(subplotnum, 1, 2)
    # 绘制频谱图
    plt.specgram(data, Fs=sample_rate, NFFT=nfft, noverlap=noverlap, cmap=get_adjusted_inferno_cmap(90, 200))
    plt.yscale('log', base=4)
    plt.ylim(1000, sample_rate / 2)
    plt.title(f'Spectrogram, NFFT={nfft}, noverlap={noverlap}')
    plt.xlabel('Time [s]')
    plt.ylabel('Frequency [Hz]')
    
    def generate_freq_yticks_nk(freq_nk_list : list[int]) -> tuple[list[int], list[str]]:
        ret = ([], [])
        for n in freq_nk_list:
            ret[0].append(n*1000)
            ret[1].append(str(n) + 'k')
        return ret
    
    tickv, ticks = generate_freq_yticks_nk([1, 2, 5, 8, 10, 16, 25, 80])
    plt.yticks(tickv, ticks)
    plt.tick_params(axis='y', which='major', labelsize=7)
    
    def band_filter(input, low, high, sample_rate):
        nyquist = 0.5 * sample_rate
        low = low / nyquist
        high = high / nyquist
        b, a = scipy.signal.butter(5, [low, high], btype='band')
        return scipy.signal.lfilter(b, a, input)

    # 对数据进行滤波
    lowcut = 7000
    highcut = 20000
    filtered_data = band_filter(data, lowcut, highcut, sample_rate)
    filtered_time = time
    print(f"filtered_data shape: {filtered_data.shape}")

    plt.subplot(subplotnum, 1, 3)
    plt.plot(filtered_time, filtered_data, linewidth=0.1)
    plt.title(f'Filtered Audio Data, Bandpass {lowcut/1000} - {highcut/1000} kHz')
    plt.xlabel('Time [s]')
    plt.ylabel('Amplitude')
    plt.xlim([filtered_time[0], filtered_time[-1]])

    # 调整布局以便在右侧显示 colorbar
    plt.tight_layout(rect=[0, 0, 0.85, 1])

    # # 创建 colorbar
    # cbar_ax = plt.gcf().add_axes([0.87, 0.63, 0.03, 0.12])
    # plt.colorbar(cax=cbar_ax, label='Intensity [dB]')
    # 获取第2个subplot的位置
    pos2 = plt.subplot(subplotnum, 1, 2).get_position()

    # 创建 colorbar 并将其放置在第2个subplot的右侧
    cbar_ax = plt.gcf().add_axes([pos2.x1 + 0.01, pos2.y0, 0.02, pos2.height])
    plt.colorbar(cax=cbar_ax, label='Intensity [dB]', ticks=[-250, -200, -150, -100, -50])
    
    plt.subplots_adjust(hspace=0.5)
    for i in range(1, subplotnum + 1):
        plt.subplot(subplotnum, 1, i)
        add_grid_ticks()

    if (save_file):
        # 创建 output 目录（如果不存在）
        output_dir = folder
        os.makedirs(output_dir, exist_ok=True)

        # 保存当前图像到 output 目录
        output_filename = os.path.join(output_dir, os.path.basename(filename).replace('.wav', '.png'))
        plt.savefig(output_filename, dpi=600)

    if (show_pic):
        plt.show()      

def find_newest_file_under_folder(folder: str, ext: str) -> str:
    files = os.listdir(folder)
    files = [f for f in files if f.endswith(ext)]
    files = [os.path.join(folder, f) for f in files]
    files = sorted(files, key=lambda x: os.path.getmtime(x), reverse=True)
    return files[0]

def plot_newest_wav_file(folder: str, ext: str):
    filename = find_newest_file_under_folder(folder, ext)
    print(f"** Processing file {filename}")
    process_file(filename, show_pic=True, save_file=False, folder='output_temp1')
    print(f"** Processed file {filename}")
    
def get_files_under_folder_sorted_by_time(folder: str, ext: str) -> list[str]:
    files = os.listdir(folder)
    files = [f for f in files if f.endswith(ext)]
    files = [os.path.join(folder, f) for f in files]
    files = sorted(files, key=lambda x: os.path.getmtime(x), reverse=False)
    return files

def select_file(folder='Data/AudioRecord'):
    root = tk.Tk()
    root.withdraw()
    file_path = filedialog.askopenfilename(initialdir=folder, title="Select file", filetypes=(("wav files", "*.wav"), ("all files", "*.*")))
    return file_path

if __name__ == '__main__':
    
    # plot_newest_wav_file('Data/AudioRecord', '.wav')
    
    # files = get_files_under_folder_sorted_by_time('Data/AudioRecord', '.wav')
    
    # process_file(files[-1], show_pic=True, save_file=False, folder='output_temp1')
    
    file = select_file('Data/AudioRecord')
    print(f"** Processing file {file}")
    process_file(file, show_pic=True, save_file=False, folder='output_temp1')
    print(f"** Processed file {file}")
    
    
    # process_file('Data/AudioRecord/audio_20241224_23_34_42.pcm.wav', show_pic=True, save_file=False, folder='output_temp1')
    
    quit()
    
    # for i in range(1, 13):
    #     if (i == 9 or i == 10):
    #         continue
            
    #     print(f"** Processing file {i:03}: filename='data\轨道 1_{i:03}.wav'")
    #     process_file(f'data\轨道 1_{i:03}.wav', show_pic=False, save_file=True, folder='output5')