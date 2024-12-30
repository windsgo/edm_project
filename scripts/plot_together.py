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
    
def get_files_under_folder_sorted_by_time(folder: str, ext: str) -> list[str]:
    files = os.listdir(folder)
    files = [f for f in files if f.endswith(ext)]
    files = [os.path.join(folder, f) for f in files]
    files = sorted(files, key=lambda x: os.path.getmtime(x), reverse=False)
    return files

def select_file(folder='Data/AudioRecord', ext='.wav'):
    root = tk.Tk()
    root.withdraw()
    # file_path = filedialog.askopenfilename(initialdir=folder, title="Select file", filetypes=(("wav files", "*.wav"), ("all files", "*.*")))
    file_path = filedialog.askopenfilename(initialdir=folder, title="Select file", filetypes=((f"{ext} files", f"*{ext}"), ("all files", "*.*")))
    return file_path

def read_motion_file(motion_file: str) -> dict:
    data_motion = {}
    header_strs = "tick_us	cmd_s	act_s	is_drilling	kn_detected	breakout_detected	detect_started	detect_state	realtime_voltage	averaged_voltage	kn	kn_valid_rate	kn_cnt"
    headers = header_strs.split('\t')
    print(headers)
    for header in headers:
        data_motion[header] = []
    with open(motion_file, 'r') as file:
        lines = file.readlines()
        print(len(lines))
    for line in lines:
        columns = line.strip().split('\t')
        for i, column in enumerate(columns):
            data_motion[headers[i]].append(column)
    print(len(data_motion))
    print(len(data_motion[headers[0]]))
    return data_motion

def read_wav_file(wav_file: str) -> tuple[int, np.ndarray]:
    sample_rate, data = wavfile.read(wav_file)
    print(f"read wav file {wav_file}, data shape: {data.shape}, sample_rate: {sample_rate}")
    
    # 获取wav文件的比特深度
    print(f"wav file {wav_file} bit depth: {data.dtype}")
    
    return sample_rate, data

def plot_together(wav_file: str, motion_file: str, save_file: bool=False, show_pic: bool=True, folder='output5'):
    # 读取音频文件
    sample_rate, wav_data = read_wav_file(wav_file)
    print(f"read wav file {wav_file}, data shape: {wav_data.shape}, sample_rate: {sample_rate}")

    # 读取运动数据文件
    motion_data = read_motion_file(motion_file)
    
    motion_ticks = motion_data["tick_us"]
    motion_time = [(int(tick) - int(motion_ticks[0])) / 1000000 for tick in motion_ticks]

    # 绘制原始数据和分贝数据
    wav_time = np.arange(wav_data.shape[0]) / sample_rate
    
    use_which_time = "wave" if (wav_time[-1] > motion_time[-1]) else "motion" # bigger
    print(f"use_which_time: {use_which_time}")

    plt.figure(figsize=(12, 10))
    
    subplotnum = 4
    
    # def add_grid_ticks():
    #     plt.xticks(np.arange(0, time[-1], 1))
    #     plt.grid(True, which='both', axis='x', linestyle='--', linewidth=0.5)

    # plot raw wave data
    plt.subplot(subplotnum, 1, 1)
    plt.plot(wav_time, wav_data, linewidth=0.1)
    plt.title('Original Audio Data')
    plt.xlabel('Time [s]')
    plt.ylabel('Amplitude')
    plt.ylim([-1, 1])
    if (use_which_time == "wave"):
        plt.xlim([wav_time[0], wav_time[-1]])
    else:
        plt.xlim([motion_time[0], motion_time[-1]])

    # plot spectrogram
    nfft = 1024
    noverlap = 512
    plt.subplot(subplotnum, 1, 2)
    plt.specgram(wav_data, Fs=sample_rate, NFFT=nfft, noverlap=noverlap, cmap=get_adjusted_inferno_cmap(90, 200))
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
    
    if (use_which_time == "wave"):
        plt.xlim([wav_time[0], wav_time[-1]])
    else:
        plt.xlim([motion_time[0], motion_time[-1]])
        
    # filter wave data and plot
    def band_filter(input, low, high, sample_rate):
        nyquist = 0.5 * sample_rate
        low = low / nyquist
        high = high / nyquist
        b, a = scipy.signal.butter(5, [low, high], btype='band')
        return scipy.signal.lfilter(b, a, input)
    
    lowcut = 7000
    highcut = 20000
    filtered_data = band_filter(wav_data, lowcut, highcut, sample_rate)
    filtered_time = wav_time
    print(f"filtered_data shape: {filtered_data.shape}")
    
    plt.subplot(subplotnum, 1, 3)
    plt.plot(filtered_time, filtered_data, linewidth=0.1)
    plt.title(f'Filtered Audio Data, Bandpass {lowcut/1000} - {highcut/1000} kHz')
    plt.xlabel('Time [s]')
    plt.ylabel('Amplitude')
    plt.ylim([-1, 1])
    if (use_which_time == "wave"):
        plt.xlim([wav_time[0], wav_time[-1]])
    else:
        plt.xlim([motion_time[0], motion_time[-1]])
    
    # rms
    # plt.subplot(subplotnum, 1, 3)
    
    # plot cmd_axis
    motion_cmd = []
    for cmd_str in motion_data["cmd_s"]:
        motion_cmd.append(float(cmd_str))
    
    plt.subplot(subplotnum, 1, 4)
    plt.plot(motion_time, motion_cmd, color='blue')
    plt.xlabel('Time [s]')
    plt.ylabel('cmd_s', color='blue')
    if (use_which_time == "wave"):
        plt.xlim([wav_time[0], wav_time[-1]])
    else:
        plt.xlim([motion_time[0], motion_time[-1]])
    
    
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
    
    plt.subplots_adjust(hspace=0.6)
    # for i in range(1, subplotnum + 1):
    #     plt.subplot(subplotnum, 1, i)
    #     add_grid_ticks()
    
    plt.show()
    
if __name__ == '__main__':
    
    # plot_newest_wav_file('Data/AudioRecord', '.wav')
    
    # files = get_files_under_folder_sorted_by_time('Data/AudioRecord', '.wav')
    
    # process_file(files[-1], show_pic=True, save_file=False, folder='output_temp1')
    
    wav_file = select_file('Data/AudioRecord', '.wav')
    print(f"** Processing file {wav_file}")
    
    motion_file = select_file('Data/MotionRecordData/Decode', '.txt')
    print(f"** Processing file {motion_file}")
    
    plot_together(wav_file, motion_file, show_pic=True, save_file=False, folder='output_temp1')