import { Injectable, NotFoundException } from '@nestjs/common';
import { Response } from 'express';
import * as fs from 'fs';
import * as path from 'path';

export interface ImageMetadata {
  filename: string;
  path: string;
  mimetype: string;
  size: number;
  timestamp: Date;
}

@Injectable()
export class ImagesService {
  private images: ImageMetadata[] = [];
  private readonly uploadsDir = './uploads';

  constructor() {
    // Create uploads directory if it doesn't exist
    if (!fs.existsSync(this.uploadsDir)) {
      fs.mkdirSync(this.uploadsDir, { recursive: true });
    }
  }

  async saveImage(imageData: ImageMetadata): Promise<ImageMetadata> {
    this.images.push(imageData);
    // In a real application, you would save this to a database
    return imageData;
  }

  async getAllImages(): Promise<ImageMetadata[]> {
    return this.images;
  }

  async getImageByFilename(filename: string, res: Response) {
    const imagePath = path.join(this.uploadsDir, filename);
    
    if (!fs.existsSync(imagePath)) {
      throw new NotFoundException('Image not found');
    }
    
    return res.sendFile(path.resolve(imagePath));
  }
} 