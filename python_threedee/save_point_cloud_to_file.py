# Copyright (C) 2022 Jason Bunk
import os
import numpy as np


def pcd_u32_colorstr(rgb):
    assert hasattr(rgb,'__len__'), str(type(rgb))+'\n'+str(rgb)[:200]
    assert len(rgb) == 3, str(len(rgb))+'\n'+str(rgb)[:200]
    for cc in rgb:
        assert isinstance(cc,int), str(type(cc))+': '+str(cc)
        assert cc >= 0 and cc <= 255, str(cc)
    return str(rgb[0]*256*256 + rgb[1]*256 + rgb[2])


def save_cloud_to_file(cloud:dict, outfile:str):
    assert isinstance(cloud,dict), str(type(cloud))+'\n'+str(cloud)[:200]
    assert isinstance(outfile,str), str(type(outfile))+'\n'+str(outfile)[:200]
    assert 'worldpoints' in cloud, str(sorted(list(cloud.keys())))
    assert '.' in os.path.basename(outfile)[-7:], outfile

    oflo = os.path.basename(outfile).lower()
    numpts = len(cloud['worldpoints'])
    have_color = ('colors' in cloud)
    if have_color:
        colors_are_float = (cloud['colors'].dtype in (np.float32, np.float64))

    if oflo.endswith('.pcd'):
        assert have_color, 'TODO: save grayscale .pcd cloud'
        with open(outfile,'w') as outfile:
            outfile.write('\n'.join([
                'VERSION .7',
                'FIELDS x y z rgb',
                'SIZE 4 4 4 4',
                'TYPE F F F U',
                'COUNT 1 1 1 1',
                f'WIDTH {numpts}',
                'HEIGHT 1',
                f'POINTS {numpts}',
                'DATA ascii',
                ])+'\n')
            for ii in range(numpts):
                outfile.write(' '.join([str(v_) for v_ in cloud['worldpoints'][ii].tolist()])+' '+pcd_u32_colorstr(cloud['colors'][ii].tolist())+'\n')

    elif oflo.endswith('.ply'):
        with open(outfile,'w') as outfile:
            outfile.write('\n'.join([
                'ply',
                'format ascii 1.0',
                f'element vertex {numpts}',
                'property float x',
                'property float y',
                'property float z',
                ])+'\n')
            if have_color:
                outfile.write('\n'.join([
                    'property uint8 red',
                    'property uint8 green',
                    'property uint8 blue',
                ])+'\n')
            outfile.write('end_header\n')
            for ii in range(numpts):
                line = ' '.join([str(v_) for v_ in cloud['worldpoints'][ii].tolist()])
                if have_color:
                    if colors_are_float:
                        line += ' ' + ' '.join([str(max(0, min(255, int(round(v_*255.))))) for v_ in cloud['colors'][ii].tolist()])
                    else:
                        line += ' ' + ' '.join([str(v_) for v_ in cloud['colors'][ii].tolist()])
                outfile.write(line+'\n')

    else:
        assert 0, 'unsupported file type: '+oflo